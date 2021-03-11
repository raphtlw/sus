# sus

Automatically suspend processes on Linux ⏸️

## About

`sus` automatically "pauses" processes when they are inactive to save resources on Linux. It does this by sending a `SIGSTOP` signal to the process to suspend the process when the window associated with the process is minimized. It sends a `SIGCONT` signal to the process when the window associated with the process is unminimized.

NOTE: This program currently only supports X11.

## Installation and Usage



## External Dependencies

- wmctrl
