// TODO: 需要传参函数的线程创建
// 按值传递
// 引用传递
// 复杂捕获
// 类成员函数
// 部分绑定std::bind + std::placeholders
// lambda表达式
/** 知识扩充
 * 1.什么是引用？和指针的区别是什么？使用引用的好处是什么？
 * a.引用是已存在变量的别名，引用只能且必须在初始化的时候被初始化一次，后面就不能再指向其他对象。
 * b.引用不能为null，指针可以为null。引用不能指向数组元素。
 * c.指针则不需要初始化，且可以随时指向其他对象。
 * d.引用不支持多级连接，使用上更安全。
 * e.从语法上来看，引用和变量使用方法一致，指针需要配合*和&运算符。
 * f.使用引用的好处是可以避免指针带来的空指针和悬空指针问题，同时语法上更简洁。
 * g.引用 和 常量指针相似，但是又有细微区别。
 * 
 * 2.复杂捕获的场景是什么？
 * 复杂捕获是指在lambda表达式中捕获多个变量，或者捕获变量的引用。
 * int a = 1, b = 2, c = 3;
 * auto lambda1 = [=]() { return a + b + c; }; // 按值捕获
 * auto lambda2 = [&]() { return a + b + c; }; // 按引用捕获
 * auto lambda3 = [c]() mutable { return a + b + c; }; // 捕获c的副本，并且可以修改
 *
 * 3.使用类成员函数创建线程和普通函数创建线程的区别是什么？
 * 使用类成员函数创建线程，需要同时把对象指针和函数指针都传入thread构造函数中。
 * 而使用普通函数创建线程，只需要把函数指针传入thread构造函数中。
 */

#include <iostream>
#include <thread>
#include <chrono>

int func(int a, int b)
{
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "thread id: " << std::this_thread::get_id() << ", a + b = " << a + b << std::endl;
    return a + b;
}

int main()
{
    std::cout << "main thread id: " << std::this_thread::get_id() << std::endl;

    int a = 10;
    int b = 20;

    std::thread t1(func, a, b); //默认值传递
    t1.join();

    // 引用传递
    std::thread t2(func, std::ref(a), std::ref(b)); 
    //ref是一个引用包装器，通常用于模板函数和移步操作(thread,bind,async)中显式传引用。
    t2.join();

    // 复杂捕获
    std::thread t3([=]() {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "thread id: " << std::this_thread::get_id() << ", a + b = " << a + b << std::endl;
    });
    t3.join();

    return 0;
}


// 思考另外几个例子，你需要时刻关注自己需要的是值传递还是引用传递。

// 希望传值但是发生了引用传递
void f(int i,std::string const& str);
void example1(int some_param) // 我们希望的是值传递，但是发生了引用传递，这会导致错误
{
    char buffer[1024];
    sprintf(buffer, "oops: %d\n", some_param);
    std::thread t(f, 3, buffer); // 这里buffer会被隐式转换成char*传递给线程函数f
    // t2 才是正确做法，转换成string类型之后，buffer的内容会被拷贝到string对象中，在线程函数f中使用的是string对象的副本。
    std::thread t2(f, 4, std::string(buffer));
    t.detach();
}

// 希望传引用但是发生了值传递
void update_data_for_widget( int widget_id, const std::vector<std::string>& data );
void example2()
{
    std::vector<std::string> data = { "data1", "data2", "data3" };
    std::thread t(update_data_for_widget, 1, data); //这里发生了值传递，但是实际我们希望是引用传递。
    std::thread t2(update_data_for_widget, 1, std::ref(data)); // 使用ref可以帮助我们显式地传递引用
    t.detach();
}

// 移动语句在线程中的应用
void example3()
{
    std::unique_ptr<std::vector<std::string>> data = std::make_unique<std::vector<std::string>>(std::initializer_list<std::string>{ "data1", "data2", "data3" });
    std::thread t(update_data_for_widget, 1, std::move(*data)); // 使用std::move将data的所有权转移给线程
    t.detach();
}
// 同理，线程也可以发生移动语义
void example4()
{
    std::unique_ptr<std::vector<std::string>> data = std::make_unique<std::vector<std::string>>(std::initializer_list<std::string>{ "data1", "data2", "data3" });
    std::thread t(update_data_for_widget, 1, std::move(*data)); // 使用std::move将data的所有权转移给线程
    t.detach();
}
