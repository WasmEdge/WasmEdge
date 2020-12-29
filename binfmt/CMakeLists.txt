# SPDX-License-Identifier: Apache-2.0

install(FILES ssvm-systemd COMPONENT static DESTINATION lib/binfmt.d/ RENAME ssvm.conf)
install(FILES ssvm-debian COMPONENT static_debian DESTINATION share/binfmts/ RENAME ssvm)
