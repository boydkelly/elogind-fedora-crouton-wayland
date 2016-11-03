/***
  This file is part of elogind.

  Copyright 2016 Daniel Mack

  elogind is free software; you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation; either version 2.1 of the License, or
  (at your option) any later version.

  elogind is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with elogind; If not, see <http://www.gnu.org/licenses/>.
***/

#include <linux/libbpf.h>
#include <string.h>
#include <unistd.h>

#include "bpf-firewall.h"
#include "bpf-program.h"
#include "load-fragment.h"
#include "manager.h"
#include "rm-rf.h"
#include "service.h"
#include "test-helper.h"
#include "tests.h"
#include "unit.h"

int main(int argc, char *argv[]) {
        struct bpf_insn exit_insn[] = {
                BPF_MOV64_IMM(BPF_REG_0, 1),
                BPF_EXIT_INSN()
        };

        _cleanup_(rm_rf_physical_and_freep) char *runtime_dir = NULL;
        CGroupContext *cc = NULL;
        _cleanup_(bpf_program_unrefp) BPFProgram *p = NULL;
        Manager *m = NULL;
        Unit *u;
        char log_buf[65535];
        int r;

        log_set_max_level(LOG_DEBUG);
        log_parse_environment();
        log_open();

        enter_cgroup_subroot();
        assert_se(set_unit_path(get_testdata_dir("")) >= 0);
        assert_se(runtime_dir = setup_fake_runtime_dir());

        r = bpf_program_new(BPF_PROG_TYPE_CGROUP_SKB, &p);
        assert(r == 0);

        r = bpf_program_add_instructions(p, exit_insn, ELEMENTSOF(exit_insn));
        assert(r == 0);

        if (getuid() != 0) {
                log_notice("Not running as root, skipping kernel related tests.");
                return EXIT_TEST_SKIP;
        }

        r = bpf_firewall_supported();
        if (r == 0) {
                log_notice("BPF firewalling not supported, skipping");
                return EXIT_TEST_SKIP;
        }
        assert_se(r > 0);

        r = bpf_program_load_kernel(p, log_buf, ELEMENTSOF(log_buf));
        assert(r >= 0);

        p = bpf_program_unref(p);

        /* The simple tests suceeded. Now let's try full unit-based use-case. */

        assert_se(manager_new(UNIT_FILE_USER, true, &m) >= 0);
        assert_se(manager_startup(m, NULL, NULL) >= 0);

        assert_se(u = unit_new(m, sizeof(Service)));
        assert_se(unit_add_name(u, "foo.service") == 0);
        assert_se(cc = unit_get_cgroup_context(u));
        u->perpetual = true;

        cc->ip_accounting = true;

        assert_se(config_parse_ip_address_access(u->id, "filename", 1, "Service", 1, "IPAddressAllow", 0, "10.0.1.0/24", &cc->ip_address_allow, NULL) == 0);
        assert_se(config_parse_ip_address_access(u->id, "filename", 1, "Service", 1, "IPAddressAllow", 0, "127.0.0.2", &cc->ip_address_allow, NULL) == 0);
        assert_se(config_parse_ip_address_access(u->id, "filename", 1, "Service", 1, "IPAddressDeny", 0, "127.0.0.3", &cc->ip_address_deny, NULL) == 0);
        assert_se(config_parse_ip_address_access(u->id, "filename", 1, "Service", 1, "IPAddressDeny", 0, "10.0.3.2/24", &cc->ip_address_deny, NULL) == 0);
        assert_se(config_parse_ip_address_access(u->id, "filename", 1, "Service", 1, "IPAddressDeny", 0, "127.0.0.1/25", &cc->ip_address_deny, NULL) == 0);
        assert_se(config_parse_ip_address_access(u->id, "filename", 1, "Service", 1, "IPAddressDeny", 0, "127.0.0.4", &cc->ip_address_deny, NULL) == 0);

        assert(cc->ip_address_allow);
        assert(cc->ip_address_allow->items_next);
        assert(!cc->ip_address_allow->items_next->items_next);

        /* The deny list is defined redundantly, let's ensure it got properly reduced */
        assert(cc->ip_address_deny);
        assert(cc->ip_address_deny->items_next);
        assert(!cc->ip_address_deny->items_next->items_next);

        assert_se(config_parse_exec(u->id, "filename", 1, "Service", 1, "ExecStart", SERVICE_EXEC_START, "/usr/bin/ping -c 1 127.0.0.2 -W 5", SERVICE(u)->exec_command, u) == 0);
        assert_se(config_parse_exec(u->id, "filename", 1, "Service", 1, "ExecStart", SERVICE_EXEC_START, "/usr/bin/ping -c 1 127.0.0.3 -W 5", SERVICE(u)->exec_command, u) == 0);

        assert_se(SERVICE(u)->exec_command[SERVICE_EXEC_START]);
        assert_se(SERVICE(u)->exec_command[SERVICE_EXEC_START]->command_next);
        assert_se(!SERVICE(u)->exec_command[SERVICE_EXEC_START]->command_next->command_next);

        SERVICE(u)->type = SERVICE_ONESHOT;
        u->load_state = UNIT_LOADED;

        unit_dump(u, stdout, NULL);

        r = bpf_firewall_compile(u);
        if (IN_SET(r, -ENOTTY, -ENOSYS, -EPERM )) {
                /* Kernel doesn't support the necessary bpf bits, or masked out via seccomp? */
                manager_free(m);
                return EXIT_TEST_SKIP;
        }
        assert_se(r >= 0);

        assert(u->ip_bpf_ingress);
        assert(u->ip_bpf_egress);

        r = bpf_program_load_kernel(u->ip_bpf_ingress, log_buf, ELEMENTSOF(log_buf));

        log_notice("log:");
        log_notice("-------");
        log_notice("%s", log_buf);
        log_notice("-------");

        assert(r >= 0);

        r = bpf_program_load_kernel(u->ip_bpf_egress, log_buf, ELEMENTSOF(log_buf));

        log_notice("log:");
        log_notice("-------");
        log_notice("%s", log_buf);
        log_notice("-------");

        assert(r >= 0);

        assert(unit_start(u) >= 0);

        while (!IN_SET(SERVICE(u)->state, SERVICE_DEAD, SERVICE_FAILED))
                assert_se(sd_event_run(m->event, UINT64_MAX) >= 0);

        assert_se(SERVICE(u)->exec_command[SERVICE_EXEC_START]->exec_status.code == CLD_EXITED &&
                  SERVICE(u)->exec_command[SERVICE_EXEC_START]->exec_status.status == EXIT_SUCCESS);

        assert_se(SERVICE(u)->exec_command[SERVICE_EXEC_START]->command_next->exec_status.code != CLD_EXITED ||
                  SERVICE(u)->exec_command[SERVICE_EXEC_START]->command_next->exec_status.status != EXIT_SUCCESS);

        manager_free(m);

        return 0;
}
