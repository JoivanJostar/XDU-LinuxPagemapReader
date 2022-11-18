# Pagemap Reader
XDU的linux内核分析作业，通过读取/proc/pid/pagemap虚拟文件来获取某个进程的虚拟页的映射情况
Pagemap文件包含每个虚拟页的映射信息，一个虚拟页对应8字节的数据，这8字节的bit含义如下：
(以下内容复制自官方文档)


```
pagemap is a new (as of 2.6.25) set of interfaces in the kernel that allow
userspace programs to examine the page tables and related information by
reading files in /proc.

There are four components to pagemap:
 * /proc/pid/pagemap.  This file lets a userspace process find out which
   physical frame each virtual page is mapped to.  It contains one 64-bit
   value for each virtual page, containing the following data (from
   fs/proc/task_mmu.c, above pagemap_read):

    * Bits 0-54  page frame number (PFN) if present
    * Bits 0-4   swap type if swapped
    * Bits 5-54  swap offset if swapped
    * Bit  55    pte is soft-dirty (see Documentation/vm/soft-dirty.txt)
    * Bit  56    page exclusively mapped (since 4.2)
    * Bits 57-60 zero
    * Bit  61    page is file-page or shared-anon (since 3.5)
    * Bit  62    page swapped
    * Bit  63    page present



   If the page is not present but in swap, then the PFN contains an
   encoding of the swap file number and the page's offset into the
   swap. Unmapped pages return a null PFN. This allows determining
   precisely which pages are mapped (or in swap) and comparing mapped
   pages between processes.

   Efficient users of this interface will use /proc/pid/maps to
   determine which areas of memory are actually mapped and llseek to
   skip over unmapped regions.
   ```
   该c程序有2个功能：
   1.convert:指定某个进程的某个虚拟地址，将其转为实际的物理地址
   
   2.stat:统计当前进程的虚拟页的映射情况，有多少个页被映射为物理页，有多少页被Swap。
   注：运行该程序需要sudo权限。 某些新版本的Linux上无法正常读取 （实测Ubuntu 20无法读取，但是Ubuntu18可以）
