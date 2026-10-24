A tree with no scanner sources in it, for the privacy check's own test
(libs/scan/tests/check_privacy_selftest.cmake): a check pointed at the wrong root would find
nothing to object to, and has to say so rather than pass.
