# mysql_modern_cpp
A simple C++ class for operating mysql database


<a href="https://996.icu"><img src="https://img.shields.io/badge/link-996.icu-red.svg" alt="996.icu" /></a>
[![996.icu](https://img.shields.io/badge/link-996.icu-red.svg)](https://996.icu)
[![LICENSE](https://img.shields.io/badge/license-Anti%20996-blue.svg)](https://github.com/996icu/996.ICU/blob/master/LICENSE)

* header only,基于C++17,依赖fmt库(fmt库也是header only的,而且fmt库是即将进入c++20标准的format库);

```c++
mysql::database db;
db.connect("localhost", "root", "123456", "mir3_user");

// R"()" 是 c++ 11 的 raw string 语法，避免字符串换行时还要在行尾添加反斜杠
db << R"(CREATE TABLE `tbl_user` (
	`name` VARCHAR(20) NOT NULL,
	`age` INT NULL DEFAULT NULL,
	`birth` DATETIME NULL DEFAULT NULL,
	PRIMARY KEY(`name`)
	)
	COLLATE = 'gbk_chinese_ci'
	ENGINE = InnoDB
	;)";

// "db << ..." 这种操作符方式会生成一个临时变量 当这个临时变量销毁时会在析构函数中自动执行sql语句
// 注意这种情况下执行sql语句时如果出现错误不会进到示例这里最后面的catch块中
db << "insert into tbl_user (name,age) values (?, ?);"
	<< "admin"
	<< 102;
db << "update tbl_user set age=?,birth=? where name=?;"
	<< nullptr
	<< nullptr
	<< "admin";
db << "update tbl_user set age=?,birth=? where name=?;"
	<< 55
	<< "1990-03-14 15:15:15"
	<< "admin";

db << "delete from tbl_user  where name=?;"
	<< "tester";

// 直接调用 db.execute 会直接执行sql语句 如果出现错误可以进到示例这里最后面的catch块中
db.execute("insert into tbl_user values (?, ?, ?);", "tester", 32, "2020-03-14 10:10:10");

std::string name, age, birth;

int count = 0;
db << "select count(*) from tbl_user;"
	>> count;

db << "select name from tbl_user where age=55;"
	>> name;

std::tm tm_birth{}; // 将获取到的日期存储到c++语言的结构体tm中
db << "select birth from tbl_user where name=?;"
	<< name
	>> tm_birth;

const char * inject_name = "admin' or 1=1 or '1=1"; // sql 注入
db << "select count(*) from tbl_user where name=?;"
	<< inject_name
	>> count;

// 将获取的内容存储到绑定数据中 这样你可以直接操作数据的缓冲区buffer
// 但此时必须要用auto rs = 这种方式将recordset临时变量保存起来
// 否则operator>>结束后临时变量就销毁了 binder 指针指向的内容就是非法的了
mysql::binder * binder = nullptr;
auto rs = db << "select birth from tbl_user where name='admin';";
rs >> binder;
MYSQL_TIME * time = (MYSQL_TIME *)(binder->buffer.get());
printf("%d-%d-%d %d:%d:%d\n", time->year, time->month, time->day, time->hour, time->minute, time->second);

// 查询到数据后直接调用 lambda 回调函数，有多少行数据，就会调用多少次
(db << "select name,age,birth from tbl_user where age>?;")
	<< 10
	>> [](std::string_view name, int age, std::string birth)
{
	printf("%s %d %s\n", name.data(), age, birth.data());
};

db << "select age,birth from tbl_user;"
	>> std::tie(age, birth);

db.execute("select name,age,birth from tbl_user where name=?;", name) >>
	[](std::string_view name, int age, std::string birth)
{
};

// set_fields_format 用来设置返回的字符串的格式，注意只有返回字符串时才起作用
// c++ 20 的format语法
// {:>15} 表示右对齐 共占15个字符的宽度
// {:04} 共占4个字符的宽度 如果不足4个字符在前面补0
rs = (db << "select name,age,birth from tbl_user;");
// 总共select了三列数据，所以set_fields_format必须要设置三个格式信息
// 可以调用set_fields_format设置格式，如果调用了就必须有几列，就填几个格式信息
// 也可以不调用set_fields_format，此时会使用默认的格式
rs.set_fields_format("{:>15}", "{:04}", "{:%Y-%m-%d %H:%M:%S}");
auto rs2 = std::move(rs);
// 按照自己的要求一行一行的获取数据
while (rs2.fetch(name, age, binder))
{
	MYSQL_TIME * time = (MYSQL_TIME *)(binder->buffer.get());
	printf("%s %s %d-%d-%d\n", name.data(), age.data(), time->year, time->month, time->day);
}

std::tuple<int, std::string> tup;
db << "select age,birth from tbl_user;"
	>> tup;
auto &[_age, _birth] = tup;
printf("%d %s\n", _age, birth.data());
```



