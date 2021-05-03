# SPDX-License-Identifier: Apache-2.0

install(FILES wasmedge-systemd COMPONENT static DESTINATION lib/binfmt.d/ RENAME wasmedge.conf)
install(FILES wasmedge-debian COMPONENT static_debian DESTINATION share/binfmts/ RENAME wasmedge)
