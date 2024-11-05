Reference: https://scoding.de/linux-kernel-exploitation-buffer_overflow

See my notes [here](https://www.notion.so/Process-and-task_struct-1348f5df87358075be32d9431aa31bd6) for more info.

This toy challenge doesn't have KASLR enabled, so our lives are much easier..

After setting up the kernel and the modules using `build.sh` and `launch.sh`, we need to find the address of `prepare_kernel_cred` and `commit_creds` in gdb.

```gdb
(remote) gef➤  p prepare_kernel_cred
$5 = {struct cred *(struct task_struct *)} 0xffffffff810b5ca0 <prepare_kernel_cred>
(remote) gef➤  p commit_creds
$6 = {int (struct cred *)} 0xffffffff810b59f0 <commit_creds>
```

We will call `commit_creds(prepare_kernel_cred(NULL))` to escalate the current user process to root.

However, since 2022-10-26, passing `NULL` as argument will generate an exception. See the patch [here](https://lore.kernel.org/lkml/87bkpxh3sz.fsf@cjr.nz/T/#Z2e.:..:20221026232943.never.775-kees::40kernel.org:1kernel:cred.c). We have to find the address of `&task_init` and pass it as argument instead. Luckily for us in this toy challenge, there is no KASLR so we can just find it in gdb.



