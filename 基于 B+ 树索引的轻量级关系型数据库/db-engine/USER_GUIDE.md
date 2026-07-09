# B+ 树数据库引擎 - 用户使用说明书

---

## 目录

1. [环境准备](#1-环境准备)
2. [编译项目](#2-编译项目)
3. [运行测试](#3-运行测试)
4. [启动数据库](#4-启动数据库)
5. [基础操作](#5-基础操作)
   - [创建表](#51-创建表)
   - [插入数据](#52-插入数据)
   - [查询数据](#53-查询数据)
   - [更新数据](#54-更新数据)
   - [删除数据](#55-删除数据)
6. [事务操作](#6-事务操作)
7. [元命令](#7-元命令)
8. [压力测试](#8-压力测试)
9. [常见问题](#9-常见问题)

---

## 1. 环境准备

### 1.1 必需软件

| 软件 | 版本要求 | 获取方式 |
|------|----------|----------|
| MinGW | 64位 | https://sourceforge.net/projects/mingw-w64/ |
| Git | 任意版本 | https://git-scm.com/download/win |

### 1.2 安装 MinGW

1. 下载 MinGW-w64 安装程序
2. 安装时选择 `x86_64-posix-seh` 架构
3. 将 MinGW 的 `bin` 目录添加到系统环境变量 `PATH`
   - 默认路径：`C:\Program Files\mingw-w64\x86_64-8.1.0-posix-seh-rt_v6-rev0\mingw64\bin`

### 1.3 验证安装

打开 PowerShell 或命令提示符，执行以下命令：

```powershell
gcc --version
```

输出示例：
```
gcc (x86_64-posix-seh-rev0, Built by MinGW-W64 project) 8.1.0
```

---

## 2. 编译项目

### 2.1 获取代码

```powershell
git clone <repository-url>
cd "d:\代码\基于 B+ 树索引的轻量级关系型数据库\db-engine"
```

### 2.2 编译全部模块

```powershell
cd "d:\代码\基于 B+ 树索引的轻量级关系型数据库\db-engine"
mingw32-make all
```

编译成功输出示例：
```
gcc -Wall -Wextra -g -I./src -c -o src/disk_manager.o src/disk_manager.c
gcc -Wall -Wextra -g -I./src -c -o src/bplus_tree.o src/bplus_tree.c
gcc -Wall -Wextra -g -I./src -c -o src/buffer_pool.o src/buffer_pool.c
gcc -Wall -Wextra -g -I./src -c -o src/lexer.o src/lexer.c
gcc -Wall -Wextra -g -I./src -c -o src/parser.o src/parser.c
gcc -Wall -Wextra -g -I./src -c -o src/executor.o src/executor.c
gcc -Wall -Wextra -g -I./src -c -o src/database.o src/database.c
gcc -Wall -Wextra -g -I./src -c -o src/wal.o src/wal.c
gcc -Wall -Wextra -g -I./src -c -o src/cli.o src/cli.c
ar rcs libdbengine.a src/disk_manager.o src/bplus_tree.o src/buffer_pool.o src/lexer.o src/parser.o src/executor.o src/database.o src/wal.o src/cli.o
gcc -Wall -Wextra -g -I./src -o dbcli src/cli.o libdbengine.a
```

### 2.3 清理编译产物

```powershell
mingw32-make clean
```

---

## 3. 运行测试

### 3.1 运行全部测试

```powershell
mingw32-make test
```

测试结果示例：
```
=== Disk Manager Tests ===   5 passed
=== B+ Tree Tests ===        19 passed
=== Buffer Pool Tests ===    9 passed
=== Lexer Tests ===          8 passed
=== Parser Tests ===         8 passed
=== Executor Tests ===       5 passed
=== Database API Tests ===   6 passed
============================
  Total:  55 passed, 0 failed
```

### 3.2 运行单项测试

```powershell
# 运行 B+ 树测试
mingw32-make test-bpt

# 运行压力测试（10万条记录）
mingw32-make test-stress
```

---

## 4. 启动数据库

### 4.1 启动命令行界面

```powershell
# 启动默认数据库（test.db）
./dbcli

# 或指定数据库文件
./dbcli mydatabase.db
```

启动成功输出：
```
B+ Tree Database Shell
Type '.help' for help, '.exit' to quit.

>
```

### 4.2 退出数据库

在命令行中输入：
```
> .exit
```

或者使用快捷键 `Ctrl+C`。

---

## 5. 基础操作

### 5.1 创建表

**语法格式**：
```sql
CREATE TABLE table_name (column_name1 TYPE, column_name2 TYPE, ...);
```

**支持的数据类型**：
| 类型 | 说明 |
|------|------|
| INT | 整数类型 |
| VARCHAR | 可变长度字符串 |
| TEXT | 文本类型 |

**示例**：

创建一个用户表：
```sql
> CREATE TABLE users (id INT, name TEXT, age INT, email VARCHAR);
Query executed.
```

创建一个商品表：
```sql
> CREATE TABLE products (id INT, name TEXT, price INT, category VARCHAR);
Query executed.
```

### 5.2 插入数据

**语法格式**：
```sql
INSERT INTO table_name (column_list) VALUES (value_list);
```

**示例**：

插入单条记录：
```sql
> INSERT INTO users (id, name, age, email) VALUES (1, 'Alice', 25, 'alice@example.com');
Query executed.
```

插入多条记录：
```sql
> INSERT INTO users (id, name, age, email) VALUES (2, 'Bob', 30, 'bob@example.com');
Query executed.
> INSERT INTO users (id, name, age, email) VALUES (3, 'Charlie', 35, 'charlie@example.com');
Query executed.
```

简化写法（省略列名，按创建时顺序）：
```sql
> INSERT INTO users VALUES (4, 'David', 40, 'david@example.com');
Query executed.
```

### 5.3 查询数据

**语法格式**：
```sql
SELECT column_list FROM table_name [WHERE condition];
```

**示例**：

查询所有记录：
```sql
> SELECT * FROM users;
*
------
1,Alice,25,alice@example.com
2,Bob,30,bob@example.com
3,Charlie,35,charlie@example.com
4,David,40,david@example.com

4 rows returned.
```

条件查询（WHERE 子句）：
```sql
> SELECT * FROM users WHERE id = 2;
*
------
2,Bob,30,bob@example.com

1 rows returned.
```

### 5.4 更新数据

**语法格式**：
```sql
UPDATE table_name SET column = value [WHERE condition];
```

**示例**：

更新单条记录：
```sql
> UPDATE users SET age = 26 WHERE id = 1;
Query executed.
```

更新多条记录：
```sql
> UPDATE users SET email = 'updated@example.com' WHERE id > 2;
Query executed.
```

验证更新结果：
```sql
> SELECT * FROM users WHERE id = 1;
*
------
1,Alice,26,alice@example.com

1 rows returned.
```

### 5.5 删除数据

**语法格式**：
```sql
DELETE FROM table_name [WHERE condition];
```

**示例**：

删除单条记录：
```sql
> DELETE FROM users WHERE id = 4;
Query executed.
```

验证删除结果：
```sql
> SELECT * FROM users;
*
------
1,Alice,26,alice@example.com
2,Bob,30,bob@example.com
3,Charlie,35,updated@example.com

3 rows returned.
```

---

## 6. 事务操作

### 6.1 开始事务

```sql
> BEGIN
Transaction started.
```

### 6.2 提交事务

```sql
> COMMIT
Transaction committed.
```

### 6.3 回滚事务

```sql
> ROLLBACK
Transaction rolled back.
```

### 6.4 事务示例

**示例：转账操作**

```sql
> CREATE TABLE accounts (id INT, name TEXT, balance INT);
Query executed.

> INSERT INTO accounts VALUES (1, 'Alice', 1000);
Query executed.

> INSERT INTO accounts VALUES (2, 'Bob', 500);
Query executed.

> SELECT * FROM accounts;
*
------
1,Alice,1000
2,Bob,500

2 rows returned.

> BEGIN
Transaction started.

> UPDATE accounts SET balance = 800 WHERE id = 1;
Query executed.

> UPDATE accounts SET balance = 700 WHERE id = 2;
Query executed.

> SELECT * FROM accounts;
*
------
1,Alice,800
2,Bob,700

2 rows returned.

> COMMIT
Transaction committed.
```

**示例：回滚操作**

```sql
> BEGIN
Transaction started.

> UPDATE accounts SET balance = 0 WHERE id = 1;
Query executed.

> SELECT * FROM accounts;
*
------
1,Alice,0
2,Bob,700

2 rows returned.

> ROLLBACK
Transaction rolled back.

> SELECT * FROM accounts;
*
------
1,Alice,800
2,Bob,700

2 rows returned.
```

---

## 7. 元命令

### 7.1 显示帮助

```sql
> .help
Available commands:
  .help          Show this help message
  .tables        List all tables
  .exit          Exit the database shell
  BEGIN          Start a transaction
  COMMIT         Commit a transaction
  ROLLBACK       Rollback a transaction
  SQL statements are executed directly

Example:
  CREATE TABLE users (id INT, name TEXT);
  INSERT INTO users VALUES (1, 'Alice');
  SELECT * FROM users;
```

### 7.2 列出所有表

```sql
> .tables
Tables (2):
  users (records: 3, columns: id, name, age, email)
  accounts (records: 2, columns: id, name, balance)
```

### 7.3 退出数据库

```sql
> .exit
Goodbye!
```

---

## 8. 压力测试

### 8.1 运行压力测试

```powershell
mingw32-make test-stress
```

### 8.2 测试内容

压力测试包含以下内容：

1. **B+ 树压力测试**（10万条记录）
   - 插入 100,000 条键
   - 查询 100 条键
   - 范围查询
   - 删除 50,000 条键

2. **SQL 管道压力测试**（10万条记录）
   - INSERT 100,000 条记录
   - SELECT WHERE 查询 100 次
   - SELECT ALL 查询全部记录
   - UPDATE 10,000 条记录
   - DELETE 10,000 条记录

3. **边界测试**
   - 边界插入
   - 删除头部记录
   - 删除尾部记录
   - 删除中间记录
   - 更新主键

### 8.3 测试结果示例

```
=== B+ Tree Stress Test ===

Insert 100000 keys: 12 ms (0.00 ms/key)
Search 100 keys: 1 ms (0.01 ms/key)
Range query (1000-2000): 0 ms
Delete 50000 keys: 4 ms (0.00 ms/key)

=== SQL Pipeline Stress Test ===

INSERT 100000 records: 173 ms (0.00 ms/record)
SELECT WHERE 100 records: 2 ms (0.02 ms/query)
SELECT ALL (100000 records): 5 ms
UPDATE 10000 records: 24 ms (0.00 ms/record)
DELETE 10000 records: 569 ms (0.06 ms/record)
Final record count: 90909

=== Edge Case Tests ===

1. Insert at boundary... OK
2. Delete first record (head)... OK
3. Delete last record (tail)... OK
4. Delete middle record... OK
5. Update primary key... OK
6. Verify remaining records... OK (8 records)

=== Summary ===
All stress tests completed successfully!
Test size: 100000 records
```

---

## 9. 常见问题

### 9.1 编译失败

**问题**：`gcc: command not found`

**解决方案**：
1. 确认 MinGW 已正确安装
2. 将 MinGW 的 `bin` 目录添加到系统环境变量 `PATH`
3. 重启命令提示符或 PowerShell

### 9.2 测试失败

**问题**：某些测试用例失败

**解决方案**：
1. 确认已执行 `mingw32-make clean` 清理旧编译产物
2. 重新执行 `mingw32-make all` 编译
3. 重新运行测试

### 9.3 数据库文件位置

**问题**：数据库文件保存在哪里？

**解决方案**：
- 默认数据库文件 `test.db` 保存在当前工作目录
- 指定数据库文件时，文件保存在指定路径
- WAL 日志文件 `xxx.db.wal` 与数据库文件在同一目录

### 9.4 数据持久化

**问题**：关闭数据库后数据是否保留？

**解决方案**：
- 当前版本为内存数据库，关闭后数据不保留
- WAL 日志用于事务恢复，但不保存实际数据
- 如需持久化，需等待后续版本的缓冲池与 B+ 树集成功能

### 9.5 性能问题

**问题**：插入或查询速度慢

**解决方案**：
- 确保使用 Release 模式编译（去掉 `-g` 参数）
- 减少不必要的查询
- 利用索引进行 WHERE 条件查询

---

## 附录：项目文件结构

```
db-engine/
├── src/
│   ├── bplus_tree.c/h      # B+树索引实现
│   ├── buffer_pool.c/h     # LRU缓冲池实现
│   ├── cli.c               # CLI命令行界面
│   ├── database.c/h        # 对外数据库API
│   ├── disk_manager.c/h    # 磁盘管理（4KB页面）
│   ├── executor.c/h        # SQL查询执行器
│   ├── lexer.c/h           # SQL词法分析器
│   ├── parser.c/h          # SQL语法分析器（AST）
│   ├── types.h             # 类型定义
│   └── wal.c/h             # WAL日志实现
├── tests/
│   ├── test_bpt.c          # B+树单元测试
│   ├── test_buffer.c       # 缓冲池单元测试
│   ├── test_database.c     # 数据库API测试
│   ├── test_disk_manager.c # 磁盘管理测试
│   ├── test_executor.c     # 执行器测试
│   ├── test_lexer.c        # 词法分析测试
│   ├── test_parser.c       # 语法分析测试
│   └── stress_test.c       # 压力测试
├── Makefile                # 编译脚本
├── TECHNICAL_DOC.md        # 技术文档
└── USER_GUIDE.md           # 用户使用说明书
```

---

## 附录：支持的 SQL 语法

### CREATE TABLE

```sql
CREATE TABLE table_name (
    column1 INT,
    column2 TEXT,
    column3 VARCHAR
);
```

### INSERT

```sql
INSERT INTO table_name (col1, col2) VALUES (val1, val2);
INSERT INTO table_name VALUES (val1, val2);
```

### SELECT

```sql
SELECT * FROM table_name;
SELECT col1, col2 FROM table_name;
SELECT * FROM table_name WHERE col = value;
```

### UPDATE

```sql
UPDATE table_name SET col = value WHERE condition;
UPDATE table_name SET col1 = val1, col2 = val2 WHERE condition;
```

### DELETE

```sql
DELETE FROM table_name;
DELETE FROM table_name WHERE condition;
```

### 事务

```sql
BEGIN;
-- 执行SQL语句
COMMIT;

BEGIN;
-- 执行SQL语句
ROLLBACK;
```

---

## 附录：性能指标

| 操作 | 数据量 | 耗时 |
|------|--------|------|
| B+树插入 | 100,000 | 12 ms |
| B+树查询 | 100 | 1 ms |
| SQL INSERT | 100,000 | 173 ms |
| SQL SELECT | 100,000 | 5 ms |
| SQL UPDATE | 10,000 | 24 ms |
| SQL DELETE | 10,000 | 569 ms |

---

**文档版本**: v1.0  
**生成日期**: 2026-07-08  
**项目**: 基于 B+ 树索引的轻量级关系型数据库
