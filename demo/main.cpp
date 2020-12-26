/*
 * > Compile:
 * g++ -c -x c++ /root/main.cpp -I /usr/local/include -I /usr/include/mysql -g2 -gdwarf-2 -o "/root/mysql_demo.o" -Wall -Wswitch -W"no-deprecated-declarations" -W"empty-body" -Wconversion -W"return-type" -Wparentheses -W"no-format" -Wuninitialized -W"unreachable-code" -W"unused-function" -W"unused-value" -W"unused-variable" -O0 -fno-strict-aliasing -fno-omit-frame-pointer -fthreadsafe-statics -fexceptions -frtti -std=c++17
 * > Link:
 * g++ -o "/root/mysql_demo.out" -Wl,--no-undefined -Wl,-L/usr/local/lib -L/usr/lib64/mysql -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack -lpthread -lrt -ldl /root/mysql_demo.o -lmysqlclient
 */

#include <cstdlib>
#include <cstdio>

#include <iostream>
#include <thread>

#include "mysql_modern_cpp.hpp"

#include <pfr/pfr.hpp>

#if defined(_MSC_VER)
#if defined(_DEBUG)
#pragma comment(lib,"libmysql.lib")
#else
#pragma comment(lib,"libmysql.lib")
#endif
#endif

using namespace std;

struct user
{
	std::string name;
	int age{};
	//std::tm birth{};
	std::chrono::system_clock::time_point birth{};

	template <class Recordset>
	bool orm(Recordset & rs)
	{
		bool result = true;
		pfr::for_each_field(*this, [&](auto& field)
		{
			result &= rs(field);
		});
		return result;
		//return rs(name, age, birth);
	}
};

int main(int argc, char* argv[])
{
#if defined(_WIN32)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try
	{
		mysql::error_code ec;
		mysql::database db;
		db.bind_error_callback([](mysql::error_code ec)
		{
			fmt::print("{} {}\n", ec.val, ec.msg);
		});
		db.connect("localhost", "root", "123456", "mir3_user");
		db.set_charset("gbk");

		(db << "show variables like 'character%';").set_fields_format("{:30}", "{}")
			>> [](std::string name, std::string value)
		{
			printf("%s %s\n", name.data(), value.data());
		};

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

		user u;
		// 查询数据到自定义结构体中
		db << "select name,age,birth from tbl_user where name=?" << "admin" >> u;
		db.execute("select name,age,birth from tbl_user where name=?", "admin").fetch(u);
		db << "select name,age,birth from tbl_user" >> [](user u)
		{
			printf("%s %d\n", u.name.data(), u.age);
		};

		// 自定义结构体的信息添加到数据库中
		u.name += std::to_string(std::rand());
		db.execute(ec, "insert into tbl_user  (name,age,birth) values (?, ?, ?);", u);
		db << "insert into tbl_user (name,age,birth) values (?,?,?)" << u;
		db << "insert into tbl_user values (?, ?, ?);" << u;

		db << "delete from tbl_user  where name=?;"
			<< "hanmeimei";

		// 直接调用 db.execute 会直接执行sql语句 如果出现错误可以进到示例这里最后面的catch块中
		db.execute(ec, "insert into tbl_user values (?, ?, ?);", "hanmeimei", 32, "2020-03-14 10:10:10");

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
	}
	catch(std::exception& e)
	{
		printf("%s\n", e.what());
	}

#if defined(_WIN32)
	system("pause");
#endif
	return 0;
}
