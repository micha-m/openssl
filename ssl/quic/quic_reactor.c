/*
 * Copyright 2022 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */
#include "internal/quic_reactor.h"

/*
 * Core I/O Reactor Framework
 * ==========================
 */
void ossl_quic_reactor_init(QUIC_REACTOR *rtor,
                            void (*tick_cb)(QUIC_TICK_RESULT *res, void *arg),
                            void *tick_cb_arg,
                            OSSL_TIME initial_tick_deadline)
{
    rtor->poll_r.type       = BIO_POLL_DESCRIPTOR_TYPE_NONE;
    rtor->poll_w.type       = BIO_POLL_DESCRIPTOR_TYPE_NONE;
    rtor->want_net_read     = 0;
    rtor->want_net_write    = 0;
    rtor->tick_deadline     = initial_tick_deadline;

    rtor->tick_cb           = tick_cb;
    rtor->tick_cb_arg       = tick_cb_arg;
}

void ossl_quic_reactor_set_poll_r(QUIC_REACTOR *rtor, const BIO_POLL_DESCRIPTOR *r)
{
    rtor->poll_r = *r;
}

void ossl_quic_reactor_set_poll_w(QUIC_REACTOR *rtor, const BIO_POLL_DESCRIPTOR *w)
{
    rtor->poll_w = *w;
}

const BIO_POLL_DESCRIPTOR *ossl_quic_reactor_get_poll_r(QUIC_REACTOR *rtor)
{
    return &rtor->poll_r;
}

const BIO_POLL_DESCRIPTOR *ossl_quic_reactor_get_poll_w(QUIC_REACTOR *rtor)
{
    return &rtor->poll_w;
}

int ossl_quic_reactor_want_net_read(QUIC_REACTOR *rtor)
{
    return rtor->want_net_read;
}

int ossl_quic_reactor_want_net_write(QUIC_REACTOR *rtor)
{
    return rtor->want_net_write;
}

OSSL_TIME ossl_quic_reactor_get_tick_deadline(QUIC_REACTOR *rtor)
{
    return rtor->tick_deadline;
}

int ossl_quic_reactor_tick(QUIC_REACTOR *rtor)
{
    QUIC_TICK_RESULT res = {0};

    /*
     * Note that the tick callback cannot fail; this is intentional. Arguably it
     * does not make that much sense for ticking to 'fail' (in the sense of an
     * explicit error indicated to the user) because ticking is by its nature
     * best effort. If something fatal happens with a connection we can report
     * it on the next actual application I/O call.
     */
    rtor->tick_cb(&res, rtor->tick_cb_arg);

    rtor->want_net_read     = res.want_net_read;
    rtor->want_net_write    = res.want_net_write;
    rtor->tick_deadline     = res.tick_deadline;
    return 1;
}

/*
 * Blocking I/O Adaptation Layer
 * =============================
 */

/*
 * Utility which can be used to poll on up to two FDs. This is designed to
 * support use of split FDs (e.g. with SSL_set_rfd and SSL_set_wfd where
 * different FDs are used for read and write).
 *
 * Generally use of poll(2) is preferred where available. Windows, however,
 * hasn't traditionally offered poll(2), only select(2). WSAPoll() was
 * introduced in Vista but has seemingly been buggy until relatively recent
 * versions of Windows 10. Moreover we support XP so this is not a suitable
 * target anyway. However, the traditional issues with select(2) turn out not to
 * be an issue on Windows; whereas traditional *NIX select(2) uses a bitmap of
 * FDs (and thus is limited in the magnitude of the FDs expressible), Windows
 * select(2) is very different. In Windows, socket handles are not allocated
 * contiguously from zero and thus this bitmap approach was infeasible. Thus in
 * adapting the Berkeley sockets API to Windows a different approach was taken
 * whereby the fd_set contains a fixed length array of socket handles and an
 * integer indicating how many entries are valid; thus Windows select()
 * ironically is actually much more like *NIX poll(2) than *NIX select(2). In
 * any case, this means that the relevant limit for Windows select() is the
 * number of FDs being polled, not the magnitude of those FDs. Since we only
 * poll for two FDs here, this limit does not concern us.
 *
 * Usage: rfd and wfd may be the same or different. Either or both may also be
 * -1. If rfd_want_read is 1, rfd is polled for readability, and if
 * wfd_want_write is 1, wfd is polled for writability. Note that since any
 * passed FD is always polled for error conditions, setting rfd_want_read=0 and
 * wfd_want_write=0 is not the same as passing -1 for both FDs.
 *
 * deadline is a timestamp to return at. If it is ossl_time_infinite(), the call
 * never times out.
 *
 * Returns 0 on error and 1 on success. Timeout expiry is considered a success
 * condition. We don't elaborate our return values here because the way we are
 * actually using this doesn't currently care.
 */
static int poll_two_fds(int rfd, int rfd_want_read,
                        int wfd, int wfd_want_write,
                        OSSL_TIME deadline)
{
#if defined(OSSL_SYS_WINDOWS) || !defined(POLLIN)
    fd_set rfd_set, wfd_set, efd_set;
    OSSL_TIME now, timeout;
    struct timeval tv, *ptv;
    int maxfd, pres;

#ifndef OSSL_SYS_WINDOWS
    /*
     * On Windows there is no relevant limit to the magnitude of a fd value (see
     * above). On *NIX the fd_set uses a bitmap and we must check the limit.
     */
    if (rfd >= FD_SETSIZE || wfd >= FD_SETSIZE)
        return 0;
#endif

    FD_ZERO(&rfd_set);
    FD_ZERO(&wfd_set);
    FD_ZERO(&efd_set);

    if (rfd != -1 && rfd_want_read)
        openssl_fdset(rfd, &rfd_set);
    if (wfd != -1 && wfd_want_write)
        openssl_fdset(wfd, &wfd_set);

    /* Always check for error conditions. */
    if (rfd != -1)
        openssl_fdset(rfd, &efd_set);
    if (wfd != -1)
        openssl_fdset(wfd, &efd_set);

    maxfd = rfd;
    if (wfd > maxfd)
        maxfd = wfd;

    if (rfd == -1 && wfd == -1 && ossl_time_is_infinite(deadline))
        /* Do not block forever; should not happen. */
        return 0;

    do {
        /*
         * select expects a timeout, not a deadline, so do the conversion.
         * Update for each call to ensure the correct value is used if we repeat
         * due to EINTR.
         */
        if (ossl_time_is_infinite(deadline)) {
            ptv = NULL;
        } else {
            now = ossl_time_now();
            /*
             * ossl_time_subtract saturates to zero so we don't need to check if
             * now > deadline.
             */
            timeout = ossl_time_subtract(deadline, now);
            tv      = ossl_time_to_timeval(timeout);
            ptv     = &tv;
        }

        pres = select(maxfd + 1, &rfd_set, &wfd_set, &efd_set, ptv);
    } while (pres == -1 && get_last_socket_error_is_eintr());

    return pres < 0 ? 0 : 1;
#else
    int pres, timeout_ms;
    OSSL_TIME now, timeout;
    struct pollfd pfds[2] = {0};
    size_t npfd = 0;

    if (rfd == wfd) {
        pfds[npfd].fd = rfd;
        pfds[npfd].events = (rfd_want_read  ? POLLIN  : 0)
                          | (wfd_want_write ? POLLOUT : 0);
        if (rfd >= 0 && pfds[npfd].events != 0)
            ++npfd;
    } else {
        pfds[npfd].fd     = rfd;
        pfds[npfd].events = (rfd_want_read ? POLLIN : 0);
        if (rfd >= 0 && pfds[npfd].events != 0)
            ++npfd;

        pfds[npfd].fd     = wfd;
        pfds[npfd].events = (wfd_want_write ? POLLOUT : 0);
        if (wfd >= 0 && pfds[npfd].events != 0)
            ++npfd;
    }

    if (npfd == 0 && ossl_time_is_infinite(deadline))
        /* Do not block forever; should not happen. */
        return 0;

    do {
        if (ossl_time_is_infinite(deadline)) {
            timeout_ms = -1;
        } else {
            now         = ossl_time_now();
            timeout     = ossl_time_subtract(deadline, now);
            timeout_ms  = ossl_time2ms(timeout);
        }

        pres = poll(pfds, npfd, timeout_ms);
    } while (pres == -1 && get_last_socket_error_is_eintr());

    return pres < 0 ? 0 : 1;
#endif
}

static int poll_descriptor_to_fd(const BIO_POLL_DESCRIPTOR *d, int *fd)
{
    if (d == NULL || d->type == BIO_POLL_DESCRIPTOR_TYPE_NONE) {
        *fd = -1;
        return 1;
    }

    if (d->type != BIO_POLL_DESCRIPTOR_TYPE_SOCK_FD || d->value.fd < 0)
        return 0;

    *fd = d->value.fd;
    return 1;
}

/*
 * Poll up to two abstract poll descriptors. Currently we only support
 * poll descriptors which represent FDs.
 */
static int poll_two_descriptors(const BIO_POLL_DESCRIPTOR *r, int r_want_read,
                                const BIO_POLL_DESCRIPTOR *w, int w_want_write,
                                OSSL_TIME deadline)
{
    int rfd, wfd;

    if (!poll_descriptor_to_fd(r, &rfd)
        || !poll_descriptor_to_fd(w, &wfd))
        return 0;

    return poll_two_fds(rfd, r_want_read, wfd, w_want_write, deadline);
}

int ossl_quic_reactor_block_until_pred(QUIC_REACTOR *rtor,
                                       int (*pred)(void *arg), void *pred_arg,
                                       uint32_t flags)
{
    int res;

    for (;;) {
        if ((flags & SKIP_FIRST_TICK) != 0)
            flags &= ~SKIP_FIRST_TICK;
        else
            /* best effort */
            ossl_quic_reactor_tick(rtor);

        if ((res = pred(pred_arg)) != 0)
            return res;

        if (!poll_two_descriptors(ossl_quic_reactor_get_poll_r(rtor),
                                  ossl_quic_reactor_want_net_read(rtor),
                                  ossl_quic_reactor_get_poll_w(rtor),
                                  ossl_quic_reactor_want_net_write(rtor),
                                  ossl_quic_reactor_get_tick_deadline(rtor)))
            /*
             * We don't actually care why the call succeeded (timeout, FD
             * readiness), we just call reactor_tick and start trying to do I/O
             * things again. If poll_two_fds returns 0, this is some other
             * non-timeout failure and we should stop here.
             *
             * TODO(QUIC): In the future we could avoid unnecessary syscalls by
             * not retrying network I/O that isn't ready based on the result of
             * the poll call. However this might be difficult because it
             * requires we do the call to poll(2) or equivalent syscall
             * ourselves, whereas in the general case the application does the
             * polling and just calls SSL_tick(). Implementing this optimisation
             * in the future will probably therefore require API changes.
             */
            return 0;
    }
}
