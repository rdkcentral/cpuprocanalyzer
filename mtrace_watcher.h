#ifndef MTRACE_WATCHER_H
#define MTRACE_WATCHER_H

#ifdef __cplusplus
extern "C" {
#endif

/* ======================================================================
 * Common self-contained API  (preferred for new components)
 *
 * The ev_loop is owned internally; components do not need libev headers.
 *
 * Typical usage in a daemon's main():
 *
 *   daemonize();
 *   // ... all subsystem init ...
 *   mtrace_watcher_start();       // register watchers
 *   mtrace_watcher_run();         // blocks forever (replaces while/sleep)
 *
 * For components with their own main loop:
 *
 *   mtrace_watcher_start();
 *   while (running) {
 *       mtrace_watcher_tick();    // non-blocking poll
 *       // ... component work ...
 *   }
 *   mtrace_watcher_stop();
 * ====================================================================== */

/**
 * mtrace_watcher_start - Create an internal ev_loop and register all watchers.
 * Call once after the process has daemonized (getppid() == 1).
 * @return 0 on success, -1 on failure.
 */
int  mtrace_watcher_start(void);

/**
 * mtrace_watcher_run - Block and drive the watcher loop until the process exits.
 * Replaces a component's while(1)/sleep() main loop.
 * Calls mtrace_watcher_stop() automatically before returning.
 */
void mtrace_watcher_run(void);

/**
 * mtrace_watcher_tick - Non-blocking single-pass event dispatch.
 * Call from an existing main loop so the watchers get CPU time.
 */
void mtrace_watcher_tick(void);

/**
 * mtrace_watcher_stop - Stop all watchers and destroy the internal loop.
 * Safe to call even if mtrace_watcher_start() was never invoked.
 */
void mtrace_watcher_stop(void);

/* ======================================================================
 * Advanced API  (for components that already own an ev_loop)
 * Requires  #include <ev.h>  before this header.
 * ====================================================================== */
#ifdef EV_H_  /* ev.h defines EV_H_ as its include guard */

/**
 * mtrace_watcher_init - Attach all watchers to a caller-provided ev_loop.
 * @param loop  An initialised ev_loop; caller owns it and drives ev_run().
 * @return 0 on success, -1 if loop is NULL.
 */
int  mtrace_watcher_init(struct ev_loop *loop);

/**
 * mtrace_watcher_cleanup - Detach and stop all watchers from the loop.
 * Call before ev_loop_destroy().
 */
void mtrace_watcher_cleanup(struct ev_loop *loop);

#endif /* EV_H_ */

#ifdef __cplusplus
}
#endif

#endif /* MTRACE_WATCHER_H */
