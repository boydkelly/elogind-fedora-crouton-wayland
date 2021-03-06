# SPDX-License-Identifier: LGPL-2.1+

#if 0 /// elogind has a stub file only
# journal_client_sources = files('''
#         audit-type.c
#         audit-type.h
#         catalog.c
#         catalog.h
#         compress.c
#         compress.h
#         journal-def.h
#         journal-file.c
#         journal-file.h
#         journal-send.c
#         journal-vacuum.c
#         journal-vacuum.h
#         journal-verify.c
#         journal-verify.h
#         lookup3.c
#         lookup3.h
#         mmap-cache.c
#         mmap-cache.h
#         sd-journal.c
# '''.split())
#else // 0
journal_client_sources = files('''
        journal-def.h
        journal-file.h
        journal-internal.h
        journal-send.c
        mmap-cache.c
        mmap-cache.h
        sd-journal.c
'''.split())
#endif // 0

#if 0 /// No gcrypt in elogind needed
# if conf.get('HAVE_GCRYPT') == 1
#         journal_client_sources += files('''
#                 journal-authenticate.c
#                 journal-authenticate.h
#                 fsprg.c
#                 fsprg.h
#         '''.split())
# endif
#endif // 0

############################################################

#if 0 /// No audit in elogind journald stubs
# audit_type_includes = [config_h,
#                        missing_audit_h,
#                        'linux/audit.h']
# if conf.get('HAVE_AUDIT') == 1
#         audit_type_includes += 'libaudit.h'
# endif
#
# generate_audit_type_list = find_program('generate-audit_type-list.sh')
# audit_type_list_txt = custom_target(
#         'audit_type-list.txt',
#         output : 'audit_type-list.txt',
#         command : [generate_audit_type_list, cpp] + audit_type_includes,
#         capture : true)
#
# audit_type_to_name = custom_target(
#         'audit_type-to-name.h',
#         input : ['audit_type-to-name.awk', audit_type_list_txt],
#         output : 'audit_type-to-name.h',
#         command : [awk, '-f', '@INPUT0@', '@INPUT1@'],
#         capture : true)
#
# journal_client_sources += [audit_type_to_name]
#endif // 0

libjournal_client = static_library(
        'journal-client',
        journal_client_sources,
        include_directories : includes,
        c_args : ['-fvisibility=default'])

############################################################

#if 0 /// After the stubs lib above, elogind needs nothing else.
# libjournal_core_sources = files('''
#         journald-audit.c
#         journald-audit.h
#         journald-console.c
#         journald-console.h
#         journald-context.c
#         journald-context.h
#         journald-kmsg.c
#         journald-kmsg.h
#         journald-native.c
#         journald-native.h
#         journald-rate-limit.c
#         journald-rate-limit.h
#         journald-server.c
#         journald-server.h
#         journald-stream.c
#         journald-stream.h
#         journald-syslog.c
#         journald-syslog.h
#         journald-wall.c
#         journald-wall.h
#         journal-internal.h
# '''.split())
#
# systemd_journald_sources = files('''
#         journald.c
#         journald-server.h
# '''.split())
#
# journald_gperf_c = custom_target(
#         'journald-gperf.c',
#         input : 'journald-gperf.gperf',
#         output : 'journald-gperf.c',
#         command : [gperf, '@INPUT@', '--output-file', '@OUTPUT@'])
#
# systemd_cat_sources = files('cat.c')
#
# journalctl_sources = files('journalctl.c')
#
# if conf.get('HAVE_QRENCODE') == 1
#         journalctl_sources += files('journal-qrcode.c',
#                                     'journal-qrcode.h')
# endif
#
# install_data('journald.conf',
#              install_dir : pkgsysconfdir)
#
# if get_option('create-log-dirs')
#         meson.add_install_script(
#                 'sh', '-c',
#                 mkdir_p.format('/var/log/journal'))
#         meson.add_install_script(
#                 'sh', '-c',
#                 '''chown 0:0 $DESTDIR/var/log/journal &&
#                    chmod 755 $DESTDIR/var/log/journal || :''')
#         if get_option('adm-group')
#                 meson.add_install_script(
#                         'sh', '-c',
#                         'setfacl -nm g:adm:rx,d:g:adm:rx $DESTDIR/var/log/journal || :')
#         endif
#         if get_option('wheel-group')
#                 meson.add_install_script(
#                         'sh', '-c',
#                         'setfacl -nm g:wheel:rx,d:g:wheel:rx $DESTDIR/var/log/journal || :')
#         endif
# endif
#endif // 0
