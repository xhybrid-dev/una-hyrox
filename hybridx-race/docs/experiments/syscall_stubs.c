/* Link-time syscall stubs for the distro arm-none-eabi toolchain.
 *
 * The SDK's Sections.ld /DISCARD/s libc.a and resolves libc calls to the
 * kernel's exported addresses (libc_exports_0.0.3.ld). The distro newlib
 * still gets pulled into the link and its reentrant wrappers reference
 * _write/_close/_read/_lseek, which the kernel does not export. ST's
 * toolchain does not hit this (Docs/sdk-setup.md). These stubs satisfy the
 * link only -- the code that would call them is discarded.
 */
#include <errno.h>
#include <sys/types.h>

int _write(int fd, const char *buf, int len) { (void)fd; (void)buf; (void)len; errno = ENOSYS; return -1; }
int _close(int fd) { (void)fd; errno = ENOSYS; return -1; }
int _read(int fd, char *buf, int len) { (void)fd; (void)buf; (void)len; errno = ENOSYS; return -1; }
int _lseek(int fd, int off, int whence) { (void)fd; (void)off; (void)whence; errno = ENOSYS; return -1; }
