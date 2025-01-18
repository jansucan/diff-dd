***

**!! This is a Pre-alpha version. Expect breaking changes and bugs. !!**

***

![Status of the tests for the master branch](https://github.com/jansucan/diff-dd/actions/workflows/tests.yml/badge.svg?branch=master)

# diff-dd

This simple utility was created to reduce size of backup images of disk
partitions. It is intended to be used in conjunction with ```dd``` or similar
utility.

## Synopsis

> diff-dd help

> diff-dd version

> diff-dd backup [-B BUFFER_SIZE] -i INFILE -b BASEFILE -o OUTFILE

> diff-dd restore [-B BUFFER_SIZE] -d DIFFFILE -o OUTFILE

## Backup

Using ```diff-dd ``` for backup requires the full backup image to
exist. Differential backup is created with:

> diff-dd backup -i INFILE -b BASEFILE -o OUTFILE

The ```INFILE``` is a path to the file to backup differentially, the
```BASEFILE``` is the full image, and the ```OUTFILE``` is the file to
which the changed data of the ```INFILE```, compared to the
```BASEFILE```, their offsets, and sizes will be saved.

## Restore

The restoration means application of the changed data saved in the
```DIFFFILE```, which is the differential image, to the ```OUTFILE```:

> diff-dd restore -d DIFFFILE -o OUTFILE

## Options

```-B``` sets the size of the buffer for the data of the input and
output files (default is 4 MiB). The input data is always buffered. The
output data is buffered only in backup mode.

## Example

First, the full image of the partition to backup has to be created:

> dd bs=4M if=/dev/sda1 of=full.img

When the user decides to create the differential image, he or she runs:

> diff-dd backup -i /dev/sda1 -b full.img -o diff.img

If a data accident happens, the partition can be restored by running:

> dd bs=4M if=full.img of=/dev/sda1

> diff-dd restore -d diff.img -o /dev/sda1

The first command restores the old full image. The second one applies
the differences.
