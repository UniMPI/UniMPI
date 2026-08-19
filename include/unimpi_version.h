#ifndef UNIMPI_VERSION_H
#define UNIMPI_VERSION_H

/* 编译期目标 MPI 版本（用户可覆盖）。缺省 = 库表面支持上限（当前 3.1）。
 * 库与应用必须用完全相同的这对宏编译（CMake INTERFACE 传播保证）。 */
#ifndef UNIMPI_MPI_TARGET_VERSION
#define UNIMPI_MPI_TARGET_VERSION 3
#define UNIMPI_MPI_TARGET_SUBVERSION 1
#endif

#define UNIMPI_MPI_AT_LEAST(MJ, MN) \
    ((UNIMPI_MPI_TARGET_VERSION) > (MJ) || \
     ((UNIMPI_MPI_TARGET_VERSION) == (MJ) && \
      (UNIMPI_MPI_TARGET_SUBVERSION) >= (MN)))

#endif
