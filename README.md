# Adler-32 driver

This is a simple driver for MINIX 3 that calculates adler-32 checksum for given input.
https://en.wikipedia.org/wiki/Adler-32

## Installation

Following entry must be added to ```/etc/system.conf``` file

```
service adler
{
        system
                IRQCTL          # 19
                DEVIO           # 21
        ;
        ipc
                SYSTEM pm rs tty ds vm vfs
                pci inet lwip amddev
                ;
    uid 0;
};
```

```
# cp adler /usr/src/minix/drivers/
# cd /usr/src/minix/drivers/adler
# make
# make install
```

## Usage

```
# mknod /dev/adler c 20 0
# service up /service/adler -dev /dev/adler
```

Reading from the driver returns and resets current checksum.

```
# head -c 8 /dev/adler | xargs echo
00000001
# echo "Hello" > /dev/adler
# head -c 8 /dev/adler | xargs echo
078b01ff
# head -c 8 /dev/adler | xargs echo
00000001
```

Checksum is preserved across driver updates.

```
# echo "Hello" > /dev/adler
# service update /service/adler -dev /dev/adler
# head -c 8 /dev/adler | xargs echo
078b01ff
```
