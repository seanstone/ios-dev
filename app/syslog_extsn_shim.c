// syslog_extsn_shim.c
// Build as a dylib for iOS. We give the C function a custom ASM label
// so its exported symbol name is exactly _syslog$DARWIN_EXTSN.

void _syslog$DARWIN_EXTSN(int priority, const char *format, ...) {
    (void)priority;
    (void)format;
    // No-op: just satisfies the reference
    // (You could forward to os_log/NSLog if you want.)
}

void syslog$DARWIN_EXTSN(int priority, const char *format, ...) {
    (void)priority;
    (void)format;
    // No-op: just satisfies the reference
    // (You could forward to os_log/NSLog if you want.)
}