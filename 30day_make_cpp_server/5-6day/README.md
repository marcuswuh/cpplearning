1. 新增Channel类和EventLoop类，Channel用来连接EventLoop和Socket fd。 EventLoop用来封装Epoll接口并承担事件循环的调度。
2. 新增Server类用来提供服务器的基本能力：创建服务fd，接收新的client连接和提供读取数据能力。
3. 使用智能指针来管理原先裸指针的资源。