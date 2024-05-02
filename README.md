# TCP server/client

## Сборка проекта

```bash
cmake -B build src
cmake --build build
```

## Запуск сервера

```bash
./build/server
```

Порт задается при создании объекта TCPServer. \
Изменить порт по пути src/start/main_server.cc. \
TCPServer server(Ваш порт);

## Запуск клиента

```bash
./build/client 127.0.0.1 9000
```

IP может быть любым.

## Возможности клиента

stats - посмотреть кол-во подключений. \
count <Текст> - подсчитать кол-во букв в слове. \
<Текст> - чтобы отправить сообщения, просто его напишите. \
exit - отключиться.