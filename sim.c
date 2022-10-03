// SPDX-License-Identifier: zlib-acknowledgement

#include <linux/uinput.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <fcntl.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

void
sleep_ms(int ms)
{
  struct timespec sleep_time = {0};
  sleep_time.tv_nsec = ms * 1000000;
  struct timespec leftover_sleep_time = {0};
  nanosleep(&sleep_time, &leftover_sleep_time);
}

void 
send_event(int fd, int type, int code, int val)
{
   struct input_event ev = {0};

   ev.type = type;
   ev.code = code;
   ev.value = val;

   write(fd, &ev, sizeof(ev));
}

void
send_keypress(int fd, int key_code)
{
  send_event(fd, EV_KEY, key_code, 1);
  send_event(fd, EV_SYN, SYN_REPORT, 0);
  send_event(fd, EV_KEY, key_code, 0);
  send_event(fd, EV_SYN, SYN_REPORT, 0);
  sleep_ms(100);
}


void
register_key(int fd, int key_code)
{
  ioctl(fd, UI_SET_EVBIT, EV_KEY);
  ioctl(fd, UI_SET_KEYBIT, key_code);
}

int 
main(int argc, char *argv[])
{
  // IMPORTANT(Ryan): Normally require root priveleges to access uinput device.
  // To work around this:
  //   $(sudo groupadd -f uinput)
  //   $(sudo usermod -a "$USER" -G uinput)
  //   /etc/udev/rules.d/99-input.rules: KERNEL=="uninput",GROUP="uinput",MODE:="0660" 
  int uinput_fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
  if (uinput_fd < 0) return EXIT_FAILURE;

  register_key(uinput_fd, KEY_F11);
  register_key(uinput_fd, KEY_F12);
  register_key(uinput_fd, KEY_TAB);
  register_key(uinput_fd, KEY_ENTER);

  struct uinput_setup usetup = {0};
  usetup.id.bustype = BUS_USB;
  usetup.id.vendor = 0x1234;
  usetup.id.product = 0x5678;
  strcpy(usetup.name, "simulated-keyboard");

  ioctl(uinput_fd, UI_DEV_SETUP, &usetup);
  ioctl(uinput_fd, UI_DEV_CREATE);

  // IMPORTANT(Ryan): Give userspace time to register then listen to device
  // Tweak per system
  sleep_ms(120);

// #define REMOTE_LINUX 0 
// #define QEMU_XPACK 0
// #define QEMU_UPSTREAM 0
#define NATIVE_QT 0
// #define REMOTE_LINUX 1

#if defined(REMOTE_LINUX)
  pid_t cur_pid = vfork();
  if (cur_pid == 0)
  {
    char *my_argv[128] = {
         "/usr/bin/bash", "--rcfile", "/home/ryan/.bashrc", "-c", 
         "ssh ryan@raspi2 'cd /home/ryan/prog/personal/example; gdbserver :1234 raspi2'",
         "exit",
         NULL
    };
    // IMPORTANT(Ryan): This environment will have to be updated every time...
    char *my_env[128] = {
      "SSH_AUTH_SOCK=/tmp/ssh-6DpHes1OX2Qd/agent.1493",
      NULL,
    };
    int res = execve("/usr/bin/bash", 
        (char **)my_argv, my_env);
  }
  else
  {
    sleep_ms(50);
    send_keypress(uinput_fd, KEY_F11);
    send_keypress(uinput_fd, KEY_TAB);
    send_keypress(uinput_fd, KEY_ENTER);
  }

#endif

#if defined(QEMU_XPACK)
  pid_t cur_pid = vfork();
  if (cur_pid == 0)
  {
    char *my_argv[64] = {
         "/home/ryan/prog/cross/arm/xpack-qemu-arm-2.8.0-13/bin/qemu-system-gnuarmeclipse",
         "-S", 
         "-gdb", "tcp::1234", 
         "--nographic", 
         "--no-reboot",
         "--board", "STM32F4-Discovery",
         "--mcu", "STM32F429ZI",
         "--semihosting-config", "enable=on,target=native",
         "--image", "/home/ryan/prog/personal/tra/build/simulator-tem.elf",
         NULL
    };
    int res = execve("/home/ryan/prog/cross/arm/xpack-qemu-arm-2.8.0-13/bin/qemu-system-gnuarmeclipse", 
        (char **)my_argv, NULL);
  }
  else
  {
    sleep_ms(30);
    send_keypress(uinput_fd, KEY_F11);
    send_keypress(uinput_fd, KEY_TAB);
    send_keypress(uinput_fd, KEY_ENTER);
  }
#endif

#if defined(QEMU_UPSTREAM)
  pid_t cur_pid = fork();
  if (cur_pid == 0)
  {
    // TODO(Ryan): Enable graphical output
    char *my_argv[64] = {
         "/usr/bin/qemu-system-arm",
         "-S", 
         "-gdb", "tcp::1234", 
         "-machine", "raspi2",
         "-no-reboot",
         "-nographic",
         "-serial", "mon:stdio",
         "-kernel", "/home/ryan/prog/personal/ras/build/ras.elf",
         NULL
    };
    int res = execve("/usr/bin/qemu-system-arm", 
        (char **)my_argv, NULL);
  }
  else
  {
    sleep_ms(30);
    send_keypress(uinput_fd, KEY_F11);
    //send_keypress(uinput_fd, KEY_TAB);
    send_keypress(uinput_fd, KEY_ENTER);
  }

#endif

#if defined(NATIVE_QT)
  send_keypress(uinput_fd, KEY_F12);
  //send_keypress(uinput_fd, KEY_TAB);
  send_keypress(uinput_fd, KEY_ENTER);
#endif

  ioctl(uinput_fd, UI_DEV_DESTROY);
  close(uinput_fd);

  return EXIT_SUCCESS;
}
