***

**!! This is a Pre-alpha version. Expect changes and bugs. !!**

***

![Status of the tests for the master branch](https://github.com/jansucan/diff-dd/actions/workflows/tests.yml/badge.svg?branch=master)

# diff-dd

This simple utility was created to reduce size of backup images of disk
partitions. It is intended to be used in conjunction with ```dd``` or similar
utility. The primary concern is safe operation, so ```diff-dd``` performs many
checks of the input and output files. For example, the differential image file
is read twice when restoring it. Because of that, it is slower.

## Synopsis

> diff-dd help

> diff-dd backup [-S SECTOR_SIZE] [-B BUFFER_SIZE] -i INFILE -b BASEFILE -o OUTFILE

> diff-dd restore [-S SECTOR_SIZE] [-B BUFFER_SIZE] -d DIFFFILE -o OUTFILE

## Backup

Using ```diff-dd ``` for backup requires the full backup image to
exist. Differential backup is created with:

> diff-dd backup -i INFILE -b BASEFILE -o OUTFILE

The ```INFILE``` is a path to the file to backup differentially, the
```BASEFILE``` is the full image, and the ```OUTFILE``` is the file to
which only the changed sectors of the ```INFILE```, compared to the
```BASEFILE```, and their offsets will be saved.

## Restore

The restoration means application of the changed sectors saved in the
```DIFFFILE```, which is the differential image, to the ```OUTFILE```:

> diff-dd restore -d DIFFFILE -o OUTFILE

## Options

```-S``` sets the sector size by which the files will be processed
(default is 512 B). It can be used to control granularity of
differential backup.

```-B``` sets the size of the buffer for the sectors of the input and
output file (default is 4 MiB). The input data is always buffered. The
output data are buffered only in backup mode.

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
