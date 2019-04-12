# diff-dd

This simple utility was created to reduce size of backup images of
disk partitions. It is intended to be used in conjunction with ```dd
``` or similar utility. The primary concern is safe operation so
```diff-dd``` performs many checks of the input and output
files. Because of the it is slower or less effective,
e.g. differential image file is read twice when restoring it.

## Synopsis

> diff-dd [-s SECTOR_SIZE] [-b BUFFER_SIZE] [INFILE]  REFFILE  OUTFILE

## Usage

The utility is used for creating and restoring differential images
created by it.  Meaning of the ```INFILE``` and ```REFFILE``` depends
on whether backup mode or restore mode is requested. Providing ```INFILE```
selects the backup mode. Omitting it selects the restore mode.

## Backup

Using ```diff-dd ``` for backup requires the full backup image to
exist. Differential backup is created with:

> diff-dd INFILE REFFILE OUTFILE

The ```INFILE``` is a path to the file to backup differentially, the
```REFFILE``` is the full image, and the ```OUTFILE``` is the file to
which only the changed sectors of the ```INFILE```, compared to the
```REFFILE```, and their offsets will be saved.

## Restore

The restoration means application of the changed sectors saved in the
```REFFILE```, which is the differential image, to the ```OUTFILE```:

> diff-dd REFFILE OUTFILE

## Options

```-s``` sets the sector size by which the files will be processed
(default is 512 B). In can be used to control granularity of
differential backup.

```-b``` sets the size of the buffer for the sectors of the input and
output file (default is 4 MiB). The input data is always buffered. The
output data are buffered only in backup mode.

## Example

First, the full image of the partition to backup has to be created:

> dd bs=4M if=/dev/sda1 of=full.img

When user decides to create the differential image, he or she runs:

> diff-dd /dev/sda1 full.img diff.img

If a data accident happens, the partition will be restored by running:

> dd bs=4M if=full.img of=/dev/sda1
> diff-dd diff.dd /dev/sda1
