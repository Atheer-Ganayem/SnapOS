## Future features

It might take us 2 years to get here.

### Video

- We're gonna use the bios VGA buffer initially. After we set a solid foundation for the kernel we will set a linear buffer and make a simple GUI.

### Filesystem & IO

- In the future, we'll implement some sort of caching on read (pre fetching (spacial locality) and keeping data (temporal locality)).
- MMIO.

### Scheduler, Processes and Threads

- Support multithreading (on one core). Here we are gonna need real mutexes.
- Upgrade to a more complicated scheduler (maybe).
- Moving to MSP (yeah... its so hard i know).
