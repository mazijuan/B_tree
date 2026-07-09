# 项目演示指南

---

## 演示准备

### 1.1 环境检查

```powershell
# 进入项目目录
cd "d:\代码\基于 B+ 树索引的轻量级关系型数据库\db-engine"

# 检查是否已编译
ls dbcli.exe
```

### 1.2 重新编译（如果需要）

```powershell
mingw32-make clean
mingw32-make all
```

---

## 演示流程

### 步骤1：启动数据库

```powershell
./dbcli
```

**预期输出**：
```
B+ Tree Database Shell
Type '.help' for help, '.exit' to quit.

>
```

---

### 步骤2：显示帮助信息

```sql
> .help
```

**预期输出**：
```
Available commands:              -- 可用命令
  .help          Show this help message              -- 显示帮助信息
  .tables        List all tables                      -- 列出所有表
  .exit          Exit the database shell              -- 退出数据库命令行
  BEGIN          Start a transaction                  -- 开始事务
  COMMIT         Commit a transaction                 -- 提交事务
  ROLLBACK       Rollback a transaction               -- 回滚事务
  SQL statements are executed directly                -- SQL语句可直接执行

Example:                                              -- 示例
  CREATE TABLE users (id INT, name TEXT);             -- 创建用户表（包含id和name字段）
  INSERT INTO users VALUES (1, 'Alice');              -- 插入一条记录（id=1, name='Alice'）
  SELECT * FROM users;                                -- 查询所有记录
```

---

### 步骤3：创建表（CREATE TABLE）

```sql
> CREATE TABLE users (id INT, name TEXT, age INT, email VARCHAR);
```

**预期输出**：
```
Query executed.
```

---

### 步骤4：插入数据（INSERT）

```sql
> INSERT INTO users (id, name, age, email) VALUES (1, 'Alice', 25, 'alice@example.com');
> INSERT INTO users VALUES (2, 'Bob', 30, 'bob@example.com');
> INSERT INTO users VALUES (3, 'Charlie', 35, 'charlie@example.com');
> INSERT INTO users VALUES (4, 'David', 40, 'david@example.com');
> INSERT INTO users VALUES (5, 'Eve', 28, 'eve@example.com');
```

**预期输出**：
```
Query executed.
Query executed.
Query executed.
Query executed.
Query executed.
```

---

### 步骤5：查询数据（SELECT）

#### 5.1 查询所有记录

```sql
> SELECT * FROM users;
```

**预期输出**：
```
*
------
1,Alice,25,alice@example.com
2,Bob,30,bob@example.com
3,Charlie,35,charlie@example.com
4,David,40,david@example.com
5,Eve,28,eve@example.com

5 rows returned.
```

#### 5.2 条件查询（WHERE）

```sql
> SELECT * FROM users WHERE id = 3;
```

**预期输出**：
```
*
------
3,Charlie,35,charlie@example.com

1 rows returned.
```

---

### 步骤6：更新数据（UPDATE）

```sql
> UPDATE users SET age = 26 WHERE id = 1;
```

**预期输出**：
```
Query executed.
```

验证更新结果：
```sql
> SELECT * FROM users WHERE id = 1;
```

**预期输出**：
```
*
------
1,Alice,26,alice@example.com

1 rows returned.
```

---

### 步骤7：删除数据（DELETE）

```sql
> DELETE FROM users WHERE id = 5;
```

**预期输出**：
```
Query executed.
```

验证删除结果：
```sql
> SELECT * FROM users;
```

**预期输出**：
```
*
------
1,Alice,26,alice@example.com
2,Bob,30,bob@example.com
3,Charlie,35,charlie@example.com
4,David,40,david@example.com

4 rows returned.
```

---

### 步骤8：事务操作（BEGIN/COMMIT/ROLLBACK）

#### 8.1 事务提交演示

```sql
> BEGIN                                                -- 开始事务
Transaction started.                                   -- 事务已启动

> INSERT INTO users VALUES (5, 'Eve', 28, 'eve@example.com');  -- 插入记录5
Query executed.                                        -- 查询执行成功

> INSERT INTO users VALUES (6, 'Frank', 33, 'frank@example.com');  -- 插入记录6
Query executed.                                        -- 查询执行成功

> SELECT * FROM users;                                 -- 查询所有记录
*
------
1,Alice,26,alice@example.com                          -- 记录1
2,Bob,30,bob@example.com                              -- 记录2
3,Charlie,35,charlie@example.com                      -- 记录3
4,David,40,david@example.com                          -- 记录4
5,Eve,28,eve@example.com                              -- 记录5（刚插入）
6,Frank,33,frank@example.com                          -- 记录6（刚插入）

6 rows returned.                                       -- 返回6条记录

> COMMIT                                               -- 提交事务
Transaction committed.                                 -- 事务已提交（更改永久保存）
```

#### 8.2 事务回滚演示

```sql
> BEGIN                                                -- 开始事务
Transaction started.                                   -- 事务已启动

> DELETE FROM users WHERE id > 4;                      -- 删除id大于4的记录
Query executed.                                        -- 查询执行成功

> SELECT * FROM users;                                 -- 查询所有记录（验证删除）
*
------
1,Alice,26,alice@example.com                          -- 记录1
2,Bob,30,bob@example.com                              -- 记录2
3,Charlie,35,charlie@example.com                      -- 记录3
4,David,40,david@example.com                          -- 记录4
                                                       -- 记录5和6已被删除

4 rows returned.                                       -- 返回4条记录

> ROLLBACK                                             -- 回滚事务
Transaction rolled back.                               -- 事务已回滚（撤销所有操作）

> SELECT * FROM users;                                 -- 查询所有记录（验证回滚）
*
------
1,Alice,26,alice@example.com                          -- 记录1
2,Bob,30,bob@example.com                              -- 记录2
3,Charlie,35,charlie@example.com                      -- 记录3
4,David,40,david@example.com                          -- 记录4
5,Eve,28,eve@example.com                              -- 记录5（恢复）
6,Frank,33,frank@example.com                          -- 记录6（恢复）

6 rows returned.                                       -- 返回6条记录（记录5和6恢复）
```

---

### 步骤9：显示所有表（.tables）

```sql
> .tables
```

**预期输出**：
```
Tables (1):
  users (records: 6, columns: id, name, age, email)
```

---

### 步骤10：创建第二个表

```sql
> CREATE TABLE products (id INT, name TEXT, price INT, category VARCHAR);
> INSERT INTO products VALUES (1, 'iPhone', 5999, 'Electronics');
> INSERT INTO products VALUES (2, 'MacBook', 9999, 'Electronics');
> INSERT INTO products VALUES (3, 'Book', 59, 'Books');
> SELECT * FROM products;
```

**预期输出**：
```
*
------
1,iPhone,5999,Electronics
2,MacBook,9999,Electronics
3,Book,59,Books

3 rows returned.
```

---

### 步骤11：退出数据库

```sql
> .exit
```

**预期输出**：
```
Goodbye!
```

---

## 进阶演示（可选）

### 演示12：运行单元测试

```powershell
mingw32-make test          # 运行所有单元测试
```

**预期输出**：
```
=== Disk Manager Tests ===   5 passed       # 磁盘管理器测试：5个测试用例全部通过
=== B+ Tree Tests ===        19 passed      # B+树索引测试：19个测试用例全部通过
=== Buffer Pool Tests ===    9 passed       # 缓冲池测试：9个测试用例全部通过
=== Lexer Tests ===          8 passed       # 词法分析器测试：8个测试用例全部通过
=== Parser Tests ===         8 passed       # 语法分析器测试：8个测试用例全部通过
=== Executor Tests ===       5 passed       # 查询执行器测试：5个测试用例全部通过
=== Database API Tests ===   6 passed       # 数据库API测试：6个测试用例全部通过
============================
  Total:  55 passed, 0 failed               # 总计：55个测试用例全部通过，0个失败
```

---

### 演示13：运行压力测试（10万条记录）

```powershell
mingw32-make test-stress   # 运行压力测试（测试10万条记录）
```

**预期输出**：
```
Running Stress Tests...                     # 开始运行压力测试

=== B+ Tree Stress Test ===                 # B+树压力测试

Insert 100000 keys: 12 ms (0.00 ms/key)     # 插入10万条键：耗时12毫秒（平均每条0.00毫秒）
Search 100 keys: 1 ms (0.01 ms/key)        # 查询100条键：耗时1毫秒（平均每条0.01毫秒）
Range query (1000-2000): 0 ms              # 范围查询（键1000-2000）：耗时0毫秒
Delete 50000 keys: 4 ms (0.00 ms/key)      # 删除5万条键：耗时4毫秒（平均每条0.00毫秒）

=== SQL Pipeline Stress Test ===            # SQL管道压力测试

INSERT 100000 records: 173 ms (0.00 ms/record)      # 插入10万条记录：耗时173毫秒
SELECT WHERE 100 records: 2 ms (0.02 ms/query)     # WHERE条件查询100次：耗时2毫秒
SELECT ALL (100000 records): 5 ms                   # 查询全部10万条记录：耗时5毫秒
UPDATE 10000 records: 24 ms (0.00 ms/record)        # 更新1万条记录：耗时24毫秒
DELETE 10000 records: 569 ms (0.06 ms/record)       # 删除1万条记录：耗时569毫秒
Final record count: 90909                           # 最终记录数：90909条

=== Edge Case Tests ===                     # 边界测试

1. Insert at boundary... OK                 # 边界插入测试...通过
2. Delete first record (head)... OK         # 删除头部记录测试...通过
3. Delete last record (tail)... OK          # 删除尾部记录测试...通过
4. Delete middle record... OK               # 删除中间记录测试...通过
5. Update primary key... OK                 # 更新主键测试...通过
6. Verify remaining records... OK (8 records)        # 验证剩余记录测试...通过（8条记录）

=== Summary ===                             # 总结
All stress tests completed successfully!    # 所有压力测试完成！
Test size: 100000 records                   # 测试规模：10万条记录
```

---

## 演示脚本（一键复制）

```sql
-- ========== 演示脚本 ==========

-- 1. 显示帮助
.help

-- 2. 创建表
CREATE TABLE users (id INT, name TEXT, age INT, email VARCHAR);

-- 3. 插入数据
INSERT INTO users VALUES (1, 'Alice', 25, 'alice@example.com');
INSERT INTO users VALUES (2, 'Bob', 30, 'bob@example.com');
INSERT INTO users VALUES (3, 'Charlie', 35, 'charlie@example.com');
INSERT INTO users VALUES (4, 'David', 40, 'david@example.com');
INSERT INTO users VALUES (5, 'Eve', 28, 'eve@example.com');

-- 4. 查询所有记录
SELECT * FROM users;

-- 5. 条件查询
SELECT * FROM users WHERE id = 3;

-- 6. 更新数据
UPDATE users SET age = 26 WHERE id = 1;
SELECT * FROM users WHERE id = 1;

-- 7. 删除数据
DELETE FROM users WHERE id = 5;
SELECT * FROM users;

-- 8. 事务提交
BEGIN
INSERT INTO users VALUES (5, 'Eve', 28, 'eve@example.com');
INSERT INTO users VALUES (6, 'Frank', 33, 'frank@example.com');
SELECT * FROM users;
COMMIT

-- 9. 事务回滚
BEGIN
DELETE FROM users WHERE id > 4;
SELECT * FROM users;
ROLLBACK
SELECT * FROM users;

-- 10. 显示所有表
.tables

-- 11. 创建第二个表
CREATE TABLE products (id INT, name TEXT, price INT, category VARCHAR);
INSERT INTO products VALUES (1, 'iPhone', 5999, 'Electronics');
INSERT INTO products VALUES (2, 'MacBook', 9999, 'Electronics');
INSERT INTO products VALUES (3, 'Book', 59, 'Books');
SELECT * FROM products;

-- 12. 退出
.exit
```

---

## 演示注意事项

### 时间控制
- 基础演示（步骤1-11）：约5分钟
- 进阶演示（步骤12-13）：约3分钟
- 总计：8分钟左右

### 关键展示点
1. **功能完整性**：展示所有SQL命令和事务操作
2. **B+树索引**：SELECT WHERE 使用索引快速定位
3. **事务支持**：COMMIT 和 ROLLBACK 的对比演示
4. **CLI交互**：元命令 .help、.tables、.exit
5. **测试覆盖**：55个测试用例全部通过
6. **性能表现**：10万条记录快速插入和查询

### 可能的问题及解决
| 问题 | 解决方案 |
|------|----------|
| 编译失败 | 重新执行 `mingw32-make clean && mingw32-make all` |
| 命令无响应 | 检查是否有语法错误，使用 `.exit` 退出重试 |
| 测试失败 | 确保已清理旧编译产物，重新编译 |

---

## 演示话术建议

### 开场
> "各位老师好，我们团队实现了一个基于B+树索引的轻量级关系型数据库。现在我来为大家演示核心功能。"

### CREATE TABLE
> "首先，我们创建一个用户表，包含id、name、age、email四个字段。"

### INSERT
> "接下来插入几条测试数据，验证插入功能正常工作。"

### SELECT
> "现在查询所有记录，可以看到数据已正确存储。使用WHERE条件查询时，系统会利用B+树索引快速定位。"

### UPDATE/DELETE
> "更新和删除操作也能正常执行，数据会实时更新。"

### 事务演示
> "这是我们的事务功能。开启事务后，可以看到插入的数据。COMMIT后数据永久保存；而ROLLBACK则会撤销所有操作。"

### .tables
> "使用.tables命令可以查看当前数据库中的所有表及其元数据。"

### 测试结果
> "我们编写了55个单元测试，全部通过。压力测试显示，10万条记录的插入仅需173毫秒，性能符合预期。"

### 总结
> "以上就是我们项目的核心功能演示。我们实现了存储引擎、B+树索引、SQL解析器、WAL日志和CLI交互等所有基础要求。"

---

**文档版本**: v1.0  
**生成日期**: 2026-07-09  
**项目**: 基于 B+ 树索引的轻量级关系型数据库
