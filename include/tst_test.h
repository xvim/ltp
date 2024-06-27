/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2015-2016 Cyril Hrubis <chrubis@suse.cz>
 * Copyright (c) Linux Test Project, 2016-2024
 */

#ifndef TST_TEST_H__
#define TST_TEST_H__

#ifdef __TEST_H__
# error Oldlib test.h already included
#endif /* __TEST_H__ */

#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <sys/resource.h>

#include "tst_common.h"
#include "tst_res_flags.h"
#include "tst_parse.h"
#include "tst_test_macros.h"
#include "tst_checkpoint.h"
#include "tst_device.h"
#include "tst_mkfs.h"
#include "tst_fs.h"
#include "tst_pid.h"
#include "tst_cmd.h"
#include "tst_cpu.h"
#include "tst_process_state.h"
#include "tst_atomic.h"
#include "tst_kvercmp.h"
#include "tst_kernel.h"
#include "tst_minmax.h"
#include "tst_get_bad_addr.h"
#include "tst_path_has_mnt_flags.h"
#include "tst_sys_conf.h"
#include "tst_coredump.h"
#include "tst_buffers.h"
#include "tst_capability.h"
#include "tst_hugepage.h"
#include "tst_assert.h"
#include "tst_security.h"
#include "tst_taint.h"
#include "tst_memutils.h"
#include "tst_arch.h"
#include "tst_fd.h"

void tst_res_(const char *file, const int lineno, int ttype,
              const char *fmt, ...)
              __attribute__ ((format (printf, 4, 5)));

/**
 * tst_res() - Reports a test result.
 *
 * @ttype: An enum tst_res_type.
 * @arg_fmt: A printf-like format.
 * @...: A printf-like parameters.
 *
 * This is the main test reporting function. Each time this function is called
 * with one of TPASS, TFAIL, TCONF, TBROK or TWARN a counter in page of shared
 * memory is incremented. This means that there is no need to propagate test
 * results from children and that results are accounted for once this function
 * returns. The counters are incremented atomically which makes this function
 * thread-safe.
 */
#define tst_res(ttype, arg_fmt, ...) \
	({									\
		TST_RES_SUPPORTS_TCONF_TDEBUG_TFAIL_TINFO_TPASS_TWARN(\
			!((TTYPE_RESULT(ttype) ?: TCONF) & \
			(TCONF | TDEBUG | TFAIL | TINFO | TPASS | TWARN)));				\
		tst_res_(__FILE__, __LINE__, (ttype), (arg_fmt), ##__VA_ARGS__);\
	})

void tst_resm_hexd_(const char *file, const int lineno, int ttype,
	const void *buf, size_t size, const char *arg_fmt, ...)
	__attribute__ ((format (printf, 6, 7)));
/**
 * tst_res_hexd() - Reports a test result along with hex dump of a buffer.
 *
 * This call is the same as tst_res() but includes a pointer and size of the
 * buffer that is going to be printed in the output in a hexadecimal format.
 *
 * @ttype: An enum tst_res_type.
 * @buf: A pointer to a buffer to print in hexadecimal format.
 * @size: A size of the buffer.
 * @arg_fmt: A printf-like format.
 * @...: A printf-like parameters.
 */
#define tst_res_hexd(ttype, buf, size, arg_fmt, ...) \
	tst_resm_hexd_(__FILE__, __LINE__, (ttype), (buf), (size), \
			(arg_fmt), ##__VA_ARGS__)

void tst_brk_(const char *file, const int lineno, int ttype,
              const char *fmt, ...)
              __attribute__ ((format (printf, 4, 5)));

/**
 * tst_brk() - Reports a breakage and exits the test.
 *
 * @ttype: An enum tst_res_type.
 * @arg_fmt: A printf-like format.
 * @...: A printf-like parameters.
 *
 * Reports either TBROK or TCONF and exits the test immediately. When called
 * all children in the same process group as the main test library process are
 * killed. This function, unless in a test cleanup, calls _exit() and does not
 * return.
 *
 * When test is in cleanup() function TBROK is converted into TWARN by the test
 * library and we attempt to carry on with a cleanup even when tst_brk() was
 * called. This makes it possible to use SAFE_FOO() macros in the test cleanup
 * without interrupting the cleanup process on a failure.
 */
#define tst_brk(ttype, arg_fmt, ...)						\
	({									\
		TST_BRK_SUPPORTS_ONLY_TCONF_TBROK(!((ttype) &			\
			(TBROK | TCONF | TFAIL)));				\
		tst_brk_(__FILE__, __LINE__, (ttype), (arg_fmt), ##__VA_ARGS__);\
	})

void tst_printf(const char *const fmt, ...)
		__attribute__((nonnull(1), format (printf, 1, 2)));

/**
 * tst_flush() - Flushes the output file streams.
 *
 * There are rare cases when we want to flush the output file streams
 * explicitly, e.g. before we do an action that may crash the test to ensure
 * that the messages have been written out.
 *
 * This is also called by the SAFE_FORK() because otherwise each child would
 * end up with the same copy of the file in it's memory and any messages in
 * buffers would be multiplied.
 */
void tst_flush(void);

pid_t safe_fork(const char *filename, unsigned int lineno);
/**
 * SAFE_FORK() - Forks a test child.
 *
 * This call makes sure that output file streams are flushed and also handles
 * errors from fork(). Use this instead of fork() whenever possible!
 */
#define SAFE_FORK() \
	safe_fork(__FILE__, __LINE__)

#define TST_TRACE(expr)	                                            \
	({int ret = expr;                                           \
	  ret != 0 ? tst_res(TINFO, #expr " failed"), ret : ret; }) \

/**
 * tst_strerrno() - Converts an errno number into a name.
 *
 * @err: An errno number.
 * return: An errno name e.g. "EINVAL".
 */
const char *tst_strerrno(int err);

/**
 * tst_strsig() - Converts a signal number into a name.
 *
 * @sig: A signal number.
 * return: A signal name e.g. "SIGINT".
 */
const char *tst_strsig(int sig);


/**
 * tst_strstatus() - Returns string describing status as returned by wait().
 *
 * WARNING: Not thread safe.
 *
 * @status: A status as returned by wait()
 * return: A string description for the status e.g. "killed by SIGKILL".
 */
const char *tst_strstatus(int status);

#include "tst_safe_macros.h"
#include "tst_safe_file_ops.h"
#include "tst_safe_net.h"
#include "tst_clone.h"
#include "tst_cgroup.h"

/**
 * tst_reap_children() - Waits for all child processes to exit.
 *
 * Wait for all children and exit with TBROK if any of them returned a non-zero
 * exit status.
 */
void tst_reap_children(void);

/**
 * struct tst_option - Test command line option.
 *
 * @optstr: A short command line option, e.g. "a" or "a:".
 * @arg: A pointer to store the option value to.
 * @help: A help string for the option displayed when test is passed '-h' on
 *        the command-line.
 */
struct tst_option {
	char *optstr;
	char **arg;
	char *help;
};

/**
 * struct tst_tag - A test tag.
 *
 * @name: A tag name.
 * @value: A tag value.
 *
 * This structure is used to encode pointers to upstream commits in regression
 * tests as well as CVE numbers or any additional useful hints.
 *
 * The content of these tags is printed by the test on a failure to help the
 * testers with debugging.
 *
 * The supported tags are:
 *
 * - "linux-git" with first 12 numbers from an upstream kernel git hash.
 * - "CVE" with a CVE number e.g. "2000-1234".
 * - "glibc-git" with first 12 numbers from an upstream glibc git hash.
 * - "musl-git" with first 12 numbers from an upstream musl git hash.
 * - "known-fail" a message describing something that is supposed to work but
 *   rather than that produces a longstanding failures.
 */
struct tst_tag {
	const char *name;
	const char *value;
};

extern unsigned int tst_variant;

#define TST_UNLIMITED_RUNTIME (-1)

/**
 * struct tst_ulimit_val - An ulimit resource and value.
 *
 * @resource: Which resource limits should be adjusted. See setrlimit(2) for
 *            the list of the RLIMIT_* constants.
 * @rlim_cur: A limit value.
 */
struct tst_ulimit_val {
	int resource;
	rlim_t rlim_cur;
};

/**
 * struct tst_test - A test description.
 *
 * @tcnt: A number of tests. If set the test() callback is called tcnt times
 *        and each time passed an increasing counter value.
 * @options: An NULL optstr terminated array of struct tst_option.
 *
 * @min_kver: A minimal kernel version the test can run on. e.g. "3.10".
 *
 * @supported_archs: A NULL terminated array of architectures the test runs on
 *                   e.g. {"x86_64, "x86", NULL}. Calls tst_is_on_arch() to
 *                   check if current CPU architecture is supported and exits
 *                   the test with TCONF if it's not.
 *
 * @tconf_msg: If set the test exits with TCONF right after entering the test
 *             library. This is used by the TST_TEST_TCONF() macro to disable
 *             tests at compile time.
 *
 * @needs_tmpdir: If set a temporary directory is prepared for the test inside
 *                $TMPDIR and the test $CWD is set to point to it. The content
 *                of the temporary directory is removed automatically after
 *                the test is finished.
 *
 * @needs_root: If set the test exit with TCONF unless it's executed under root
 *              user.
 *
 * @forks_child: Has to be set if the test intends to fork children.
 *
 * @needs_device: If set a block device is prepared for the test, the device
 *                path and size are set in the struct tst_device variable
 *                called tst_device. If $LTP_DEV variable exists in the test
 *                environment the test attempts to use that device first and
 *                only if that fails the test falls back to use loop devices.
 *                This flag implies needs_tmpdir flag because loop device
 *                backing files are created in the test temporary directory.
 *
 * @needs_checkpoints: Has to be set if the test wants to use checkpoint
 *                     synchronization primitives.
 *
 * @needs_overlay: If set overlay file system is mounted on the top of the
 *                 file system at tst_test.mntpoint.
 *
 * @format_device: Does all tst_test.needs_device would do and also formats
 *                 the device with tst_test.dev_fs_type file system as well.
 *
 * @mount_device: Does all tst_test.format_device would do and also mounts the
 *                device at tst_test.mntpoint.
 *
 * @needs_rofs: If set a read-only file system is mounted at tst_test.mntpoint.
 *
 * @child_needs_reinit: Has to be set if the test needs to call tst_reinit()
 *                      from a process started by exec().
 *
 * @needs_devfs: If set the devfs is mounted at tst_test.mntpoint. This is
 *               needed for tests that need to create device files since tmpfs
 *               at /tmp is usually mounted with 'nodev' option.
 *
 * @restore_wallclock: Saves wall clock at the start of the test and restores
 *                     it at the end with the help of monotonic timers.
 *                     Testcases that modify system wallclock use this to
 *                     restore the system to the previous state.
 *
 * @all_filesystems: If set the test is executed for all supported filesytems,
 *                   i.e. file system that is supported by the kernel and has
 *                   mkfs installed on the system.The file system is mounted at
 *                   tst_test.mntpoint and file system details, e.g. type are set
 *                   in the struct tst_device. Each execution is independent,
 *                   that means that for each iteration tst_test.setup() is
 *                   called at the test start and tst_test.cleanup() is called
 *                   at the end and tst_brk() only exits test for a single
 *                   file system. That especially means that calling
 *                   tst_brk(TCONF, ...) in the test setup will skip the
 *                   current file system.
 *
 * @skip_in_lockdown: Skip the test if kernel lockdown is enabled.
 *
 * @skip_in_secureboot: Skip the test if secureboot is enabled.
 *
 * @skip_in_compat: Skip the test if we are executing 32bit binary on a 64bit
 *                  kernel, i.e. we are testing the kernel compat layer.
 *
 * @needs_abi_bits: Skip the test if runs on a different kernel ABI than
 *                  requested (on 32bit instead of 64bit or vice versa).
 *                  Possible values: 32, 64.
 *
 * @needs_hugetlbfs: If set hugetlbfs is mounted at tst_test.mntpoint.
 *
 * @skip_filesystems: A NULL terminated array of unsupported file systems. The
 *                    test reports TCONF if the file system to be tested is
 *                    present in the array. This is especially useful to filter
 *                    out unsupported file system when tst_test.all_filesystems
 *                    is enabled.
 *
 * @min_cpus: Minimal number of online CPUs the test needs to run.
 *
 * @min_mem_avail: Minimal amount of available RAM memory in megabytes required
 *                 for the test to run.
 *
 * @min_swap_avail: Minimal amount of available swap memory in megabytes
 *                  required for the test to run.
 *
 * @hugepages: An interface to reserve hugepages prior running the test.
 *             Request how many hugepages should be reserved in the global
 *             pool and also if having hugepages is required for the test run
 *             or not, i.e. if test should exit with TCONF if the requested
 *             amount of hugepages cannot be reserved. If TST_REQUEST is set
 *             the library will try it's best to reserve the hugepages and
 *             return the number of available hugepages in tst_hugepages, which
 *             may be 0 if there is no free memory or hugepages are not
 *             supported at all. If TST_NEEDS the requested hugepages are
 *             required for the test and the test exits if it couldn't be
 *             required. It can also be used to disable hugepages by setting
 *             .hugepages = {TST_NO_HUGEPAGES}. The test library restores the
 *             original poll allocations after the test has finished.
 *
 * @taint_check: If set the test fails if kernel is tainted during the test run.
 *               That means tst_taint_init() is called during the test setup
 *               and tst_taint_check() at the end of the test. If all_filesystems
 *               is set taint check will be performed after each iteration and
 *               testing will be terminated by TBROK if taint is detected.
 *
 * @test_variants: If set denotes number of test variant, the test is executed
 *                 variants times each time with tst_variant set to different
 *                 number. This allows us to run the same test for different
 *                 settings. The intended use is to test different syscall
 *                 wrappers/variants but the API is generic and does not limit
 *                 usage in any way.
 *
 * @dev_min_size: A minimal device size in megabytes.
 *
 * @dev_fs_type: If set overrides the default file system type for the device and
 *               sets the tst_device.fs_type.
 *
 * @dev_fs_opts: A NULL terminated array of options passed to mkfs in the case
 *               of 'tst_test.format_device'. These options are passed to mkfs
 *               before the device path.
 *
 * @dev_extra_opts: A NULL terminated array of extra options passed to mkfs in
 *                  the case of 'tst_test.format_device'. Extra options are
 *                  passed to mkfs after the device path. Commonly the option
 *                  after mkfs is the number of blocks and can be used to limit
 *                  the file system not to use the whole block device.
 *
 * @mntpoint: A mount point where the test library mounts requested file system.
 *            The directory is created by the library, the test must not create
 *            it itself.
 *
 * @mnt_flags: MS_* flags passed to mount(2) when the test library mounts a
 *             device in the case of 'tst_test.mount_device'.
 *
 * @mnt_data: The data passed to mount(2) when the test library mounts a device
 *            in the case of 'tst_test.mount_device'.
 *
 * @max_runtime: Maximal test runtime in seconds. Any test that runs for more
 *               than a second or two should set this and also use
 *               tst_remaining_runtime() to exit when runtime was used up.
 *               Tests may finish sooner, for example if requested number of
 *               iterations was reached before the runtime runs out. If test
 *               runtime cannot be know in advance it should be set to
 *               TST_UNLIMITED_RUNTIME.
 *
 * @setup: Setup callback is called once at the start of the test in order to
 *         prepare the test environment.
 *
 * @cleanup: Cleanup callback is either called at the end of the test, or in a
 *           case that tst_brk() was called. That means that cleanup must be
 *           able to handle all possible states the test can be in. This
 *           usually means that we have to check if file descriptor was opened
 *           before we attempt to close it, etc.
 *
 *
 * @test: A main test function, only one of the tst_test.test and test_all can
 *        be set. When this function is set the tst_test.tcnt must be set to a
 *        positive integer and this function will be executed tcnt times
 *        during a single test iteration. May be executed several times if test
 *        was passed '-i' or '-d' command line parameters.
 *
 * @test_all: A main test function, only one of the tst_test.test and test_all
 *            can be set. May be executed several times if test was passed '-i'
 *            or '-d' command line parameters.
 *
 * @resource_files: A NULL terminated array of filenames that will be copied
 *                  to the test temporary directory from the LTP datafiles
 *                  directory.
 *
 * @needs_drivers: A NULL terminated array of kernel modules required to run
 *                 the test. The module has to be build in or present in order
 *                 for the test to run.
 *
 * @save_restore: A {} terminated array of /proc or /sys files that should
 *                saved at the start of the test and restored at the end. See
 *                tst_sys_conf_save() and struct tst_path_val for details.
 *
 * @ulimit: A {} terminated array of process limits RLIMIT_* to be adjusted for
 *          the test.
 *
 * @needs_kconfigs: A NULL terminated array of kernel config options that are
 *                  required for the test. All strings in the array have to be
 *                  evaluated to true for the test to run. Boolean operators
 *                  and parenthesis are supported, e.g.
 *                  "CONFIG_X86_INTEL_UMIP=y | CONFIG_X86_UIMP=y" is evaluated
 *                  to true if at least one of the options is present.
 *
 * @bufs: A description of guarded buffers to be allocated for the test. Guarded
 *        buffers are buffers with poisoned page allocated right before the start
 *        of the buffer and canary right after the end of the buffer. See
 *        struct tst_buffers and tst_buffer_alloc() for details.
 *
 * @caps: A {} terminated array of capabilities to change before the start of
 *        the test. See struct tst_cap and tst_cap_setup() for details.
 *
 * @tags: A {} terminated array of test tags. See struct tst_tag for details.
 *
 * @needs_cmds: A NULL terminated array of commands required for the test to run.
 *
 * @needs_cgroup_ver: If set the test will run only if the specified cgroup
 *                    version is present on the system.
 *
 * @needs_cgroup_ctrls: A {} terminated array of cgroup controllers the test
 *                      needs to run.
 */

 struct tst_test {
	unsigned int tcnt;

	struct tst_option *options;

	const char *min_kver;

	const char *const *supported_archs;

	const char *tconf_msg;

	unsigned int needs_tmpdir:1;
	unsigned int needs_root:1;
	unsigned int forks_child:1;
	unsigned int needs_device:1;
	unsigned int needs_checkpoints:1;
	unsigned int needs_overlay:1;
	unsigned int format_device:1;
	unsigned int mount_device:1;
	unsigned int needs_rofs:1;
	unsigned int child_needs_reinit:1;
	unsigned int needs_devfs:1;
	unsigned int restore_wallclock:1;

	unsigned int all_filesystems:1;

	unsigned int skip_in_lockdown:1;
	unsigned int skip_in_secureboot:1;
	unsigned int skip_in_compat:1;
	int needs_abi_bits;

	unsigned int needs_hugetlbfs:1;

	const char *const *skip_filesystems;

	unsigned long min_cpus;
	unsigned long min_mem_avail;
	unsigned long min_swap_avail;

	struct tst_hugepage hugepages;

	unsigned int taint_check;

	unsigned int test_variants;

	unsigned int dev_min_size;

	const char *dev_fs_type;

	const char *const *dev_fs_opts;
	const char *const *dev_extra_opts;

	const char *mntpoint;
	unsigned int mnt_flags;
	void *mnt_data;

	int max_runtime;

	void (*setup)(void);
	void (*cleanup)(void);
	void (*test)(unsigned int test_nr);
	void (*test_all)(void);

	const char *scall;
	int (*sample)(int clk_id, long long usec);

	const char *const *resource_files;
	const char * const *needs_drivers;

	const struct tst_path_val *save_restore;

	const struct tst_ulimit_val *ulimit;

	const char *const *needs_kconfigs;

	struct tst_buffers *bufs;

	struct tst_cap *caps;

	const struct tst_tag *tags;

	const char *const *needs_cmds;

	const enum tst_cg_ver needs_cgroup_ver;

	const char *const *needs_cgroup_ctrls;
};

/**
 * tst_run_tcases() - Entry point to the test library.
 *
 * @argc: An argc that was passed to the main().
 * @argv: An argv that was passed to the main().
 * @self: The test description and callbacks packed in the struct tst_test
 *        structure.
 *
 * A main() function which calls this function is added to each test
 * automatically by including the tst_test.h header. The test has to define the
 * struct tst_test called test.
 *
 * This function does not return, i.e. calls exit() after the test has finished.
 */
void tst_run_tcases(int argc, char *argv[], struct tst_test *self)
                    __attribute__ ((noreturn));

#define IPC_ENV_VAR "LTP_IPC_PATH"

/**
 * tst_reinit() - Reinitialize the test library.
 *
 * In a cases where a test child process calls exec() it no longer can access
 * the test library shared memory and therefore use the test reporting
 * functions, checkpoint library, etc. This function re-initializes the test
 * library so that it can be used again.
 *
 * @important The LTP_IPC_PATH variable must be passed to the program environment.
 */
void tst_reinit(void);

unsigned int tst_multiply_timeout(unsigned int timeout);

/*
 * Returns remaining test runtime. Test that runs for more than a few seconds
 * should check if they should exit by calling this function regularly.
 *
 * The function returns remaining runtime in seconds. If runtime was used up
 * zero is returned.
 */
unsigned int tst_remaining_runtime(void);

/*
 * Sets maximal test runtime in seconds.
 */
void tst_set_max_runtime(int max_runtime);

/*
 * Create and open a random file inside the given dir path.
 * It unlinks the file after opening and return file descriptor.
 */
int tst_creat_unlinked(const char *path, int flags);

/*
 * Returns path to the test temporary directory in a newly allocated buffer.
 */
char *tst_get_tmpdir(void);

/*
 * Returns path to the test temporary directory root (TMPDIR).
 */
const char *tst_get_tmpdir_root(void);

/*
 * Validates exit status of child processes
 */
int tst_validate_children_(const char *file, const int lineno,
	unsigned int count);
#define tst_validate_children(child_count) \
	tst_validate_children_(__FILE__, __LINE__, (child_count))

#ifndef TST_NO_DEFAULT_MAIN

static struct tst_test test;

int main(int argc, char *argv[])
{
	tst_run_tcases(argc, argv, &test);
}

#endif /* TST_NO_DEFAULT_MAIN */

/**
 * TST_TEST_TCONF() - Exit tests with a TCONF message.
 *
 * This macro is used in test that couldn't be compiled either because current
 * CPU architecture is unsupported or because of missing development libraries.
 */
#define TST_TEST_TCONF(message)                                 \
        static struct tst_test test = { .tconf_msg = message  } \

#endif	/* TST_TEST_H__ */
