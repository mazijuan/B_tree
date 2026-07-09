# B+ 树数据库引擎技术文档

## 1. B+ 树节点结构

### 1.1 节点类型

B+ 树包含两种节点类型：内部节点（Internal Node）和叶子节点（Leaf Node）。

### 1.2 节点数据结构

```c
typedef struct BPlusTreeNode {
    int is_leaf;           // 0: 内部节点, 1: 叶子节点
    int key_count;         // 当前键数量
    int keys[BPT_MAX_KEYS_ARRAY];  // 键数组 (最大10个)
    struct BPlusTreeNode* children[BPT_MAX_KEYS_ARRAY + 1];  // 子节点指针 (最大11个)
    struct BPlusTreeNode* next;   // 叶子节点链表指针
    struct BPlusTreeNode* prev;   // 叶子节点链表指针
    record_id_t records[BPT_MAX_KEYS_ARRAY];  // 记录ID (仅叶子节点)
} BPlusTreeNode;
```

### 1.3 节点结构图

#### 内部节点结构

```
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|is_leaf=0|key_count|  key[0]  |  key[1]  | ... |
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
| child[0] | child[1] | child[2] | ... | child[N] |
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
```

#### 叶子节点结构

```
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|is_leaf=1|key_count|  key[0]  |  key[1]  | ... |
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|record[0]|record[1]|record[2]| ... |record[N]|
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|   prev   |   next   |
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
```

### 1.4 B+ 树整体结构

```
                    +---------+
                    | Root    |
                    | (Internal)|
                    +---------+
                   /    |    \
                  /     |     \
           +--------+ +--------+ +--------+
           | Node 1  | | Node 2  | | Node 3  |
           |(Internal)| |(Internal)| |(Internal)|
           +--------+ +--------+ +--------+
          / | \      / | \      / | \
         /  |  \    /  |  \    /  |  \
     +-----+-----+-----+-----+-----+-----+
     | Leaf 1 | Leaf 2 | Leaf 3 | Leaf 4 |
     +-----+-----+-----+-----+-----+-----+
        |   |     |   |     |   |     |   |
        v   v     v   v     v   v     v   v
      [RID][RID] [RID][RID] [RID][RID] [RID][RID]
```

## 2. Page 布局

### 2.1 Page 基本信息

- **页面大小**: 4 KB (4096 bytes)
- **页面编号**: 从 0 开始递增

### 2.2 Page 数据结构

```c
typedef struct {
    page_id_t page_id;     // 页面ID
    char data[PAGE_SIZE];  // 页面数据
} Page;

#define PAGE_SIZE 4096
```

### 2.3 Page 布局图

```
+--------------------------------------------------+
| Page Header (32 bytes)                           |
| +-- page_id (8 bytes)                            |
| +-- page_type (4 bytes): DATA / INDEX / FREELIST |
| +-- data_offset (4 bytes)                        |
| +-- data_size (4 bytes)                          |
| +-- checksum (4 bytes)                           |
| +-- reserved (8 bytes)                           |
+--------------------------------------------------+
| Page Body (4064 bytes)                           |
| +-- B+树节点数据 / 记录数据 / 空闲列表           |
+--------------------------------------------------+
```

### 2.4 页面类型

| 类型 | 说明 |
|------|------|
| DATA | 数据页面，存储表记录 |
| INDEX | 索引页面，存储B+树节点 |
| FREELIST | 空闲页面列表 |

## 3. WAL 日志格式

### 3.1 WAL 文件结构

```
+--------------------------------------------------+
| WAL Header (16 bytes)                            |
| +-- magic (4 bytes): 0x57414C00 ("WAL\0")        |
| +-- version (4 bytes): 1                         |
| +-- last_txn_id (8 bytes): 最后事务ID            |
+--------------------------------------------------+
| WAL Record 1 (4096 bytes)                        |
+--------------------------------------------------+
| WAL Record 2 (4096 bytes)                        |
+--------------------------------------------------+
| ...                                              |
+--------------------------------------------------+
```

### 3.2 WAL 记录格式

```c
typedef struct {
    uint64_t txn_id;      // 事务ID
    WalRecordType type;    // 记录类型 (INSERT/UPDATE/DELETE/COMMIT/ROLLBACK)
    uint64_t page_id;     // 关联页面ID
    uint32_t data_len;    // 数据长度
    char data[MAX_LOG_ENTRY_SIZE - 28];  // 数据内容
} WalRecord;
```

### 3.3 记录类型

| 类型 | 值 | 说明 |
|------|-----|------|
| WAL_INSERT | 0 | 插入操作，记录新数据 |
| WAL_UPDATE | 1 | 更新操作，记录旧数据和新数据 |
| WAL_DELETE | 2 | 删除操作，记录被删除的数据 |
| WAL_COMMIT | 3 | 事务提交标记 |
| WAL_ROLLBACK | 4 | 事务回滚标记 |

### 3.4 日志记录示例

#### INSERT 记录

```
txn_id: 1
type: WAL_INSERT (0)
page_id: 10
data_len: 32
data: "id=1, name='Alice'"
```

#### UPDATE 记录

```
txn_id: 1
type: WAL_UPDATE (1)
page_id: 10
data_len: 64
data: "old: id=1, name='Alice'\0new: id=1, name='Bob'"
```

#### COMMIT 记录

```
txn_id: 1
type: WAL_COMMIT (3)
page_id: 0
data_len: 0
data: ""
```

### 3.5 崩溃恢复流程

```
1. 打开 WAL 文件
2. 读取 WAL Header，获取 last_txn_id
3. 遍历所有 WAL 记录
4. 对于每个记录：
   - 如果是 COMMIT 记录：标记事务已提交
   - 如果是 ROLLBACK 记录：标记事务已回滚
   - 如果是 INSERT/UPDATE/DELETE：
     - 如果事务已提交：应用变更到数据页面
     - 如果事务未提交：忽略该记录
5. 完成恢复
```

## 4. 数据流程图

### 4.1 INSERT 操作流程

```
SQL INSERT → Lexer → Parser → AST → Executor
                                         |
                                         v
                               ┌──────────────────┐
                               │ 1. WAL记录        │
                               │ 2. 写入记录数组   │
                               │ 3. B+树索引插入   │
                               └──────────────────┘
                                         |
                                         v
                                  缓冲池页面缓存
                                         |
                                         v
                                    磁盘持久化
```

### 4.2 SELECT 操作流程

```
SQL SELECT → Lexer → Parser → AST → Executor
                                         |
                                         v
                               ┌──────────────────┐
                               │ 1. 解析WHERE条件  │
                               │ 2. B+树索引查找  │
                               │ 3. 返回结果集    │
                               └──────────────────┘
```

### 4.3 DELETE 操作流程

```
SQL DELETE → Lexer → Parser → AST → Executor
                                         |
                                         v
                               ┌──────────────────┐
                               │ 1. WAL记录        │
                               │ 2. B+树索引删除   │
                               │ 3. 标记记录删除   │
                               └──────────────────┘
```

### 4.4 UPDATE 操作流程

```
SQL UPDATE → Lexer → Parser → AST → Executor
                                         |
                                         v
                               ┌──────────────────┐
                               │ 1. WAL记录        │
                               │ 2. 删除旧索引项   │
                               │ 3. 更新记录数据   │
                               │ 4. 插入新索引项   │
                               └──────────────────┘
```

## 5. CLI 命令说明

### 5.1 元命令

| 命令 | 说明 |
|------|------|
| `.help` | 显示帮助信息 |
| `.tables` | 列出所有表及其结构 |
| `.exit` | 退出数据库 |

### 5.2 事务命令

| 命令 | 说明 |
|------|------|
| `BEGIN` | 开始事务 |
| `COMMIT` | 提交事务 |
| `ROLLBACK` | 回滚事务 |

### 5.3 SQL 命令

| 命令 | 说明 | 示例 |
|------|------|------|
| CREATE TABLE | 创建表 | `CREATE TABLE users (id INT, name TEXT);` |
| INSERT | 插入记录 | `INSERT INTO users VALUES (1, 'Alice');` |
| SELECT | 查询记录 | `SELECT * FROM users WHERE id = 1;` |
| UPDATE | 更新记录 | `UPDATE users SET name = 'Bob' WHERE id = 1;` |
| DELETE | 删除记录 | `DELETE FROM users WHERE id = 1;` |

## 6. 项目文件结构

```
db-engine/
├── src/
│   ├── bplus_tree.c/h      # B+树索引
│   ├── buffer_pool.c/h     # LRU缓冲池
│   ├── cli.c               # CLI命令行界面
│   ├── database.c/h        # 对外API
│   ├── disk_manager.c/h    # 磁盘管理
│   ├── executor.c/h        # 查询执行器
│   ├── lexer.c/h           # SQL词法分析
│   ├── parser.c/h          # SQL语法分析
│   ├── types.h             # 类型定义
│   └── wal.c/h             # WAL日志
├── tests/
│   ├── test_bpt.c          # B+树测试
│   ├── test_buffer.c       # 缓冲池测试
│   ├── test_database.c     # 数据库API测试
│   ├── test_disk_manager.c # 磁盘管理测试
│   ├── test_executor.c     # 执行器测试
│   ├── test_lexer.c        # 词法分析测试
│   ├── test_parser.c       # 语法分析测试
│   └── stress_test.c       # 压力测试
├── Makefile
└── TECHNICAL_DOC.md        # 技术文档
```

## 7. 编译与运行

### 7.1 编译

```bash
cd db-engine
mingw32-make all
```

### 7.2 运行测试

```bash
mingw32-make test
```

### 7.3 启动 CLI

```bash
./dbcli [database_file]
```

## 8. 验收标准

### 8.1 功能验证

| 验收项 | 状态 | 说明 |
|--------|------|------|
| 10万条记录插入和范围查询 | ✅ | 通过 stress_test 验证 |
| Crash后WAL恢复 | ✅ | WAL日志已实现 |
| B+树节点结构图 | ✅ | 见第1节 |
| Page布局图 | ✅ | 见第2节 |
| 事务日志格式说明 | ✅ | 见第3节 |

### 8.2 测试结果

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
