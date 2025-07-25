⚙️ Установка и запуск
Выберите инструкцию для вашей операционной системы.
Установка для Windows
1. Системные требования
Вам понадобятся следующие инструменты:
Visual Studio 2019/2022:
Установите Visual Studio Community с официального сайта.
В Visual Studio Installer выберите рабочую нагрузку "Разработка классических приложений на C++". Убедитесь, что в неё включены компоненты MSVC, CMake и Git.
Qt 6.5:
Скачайте Qt Online Installer.
В процессе установки выберите Qt 6.5 и компоненты:
MSVC 2019 64-bit (или новее, в зависимости от вашей версии Visual Studio).
Qt Core, Qt Widgets, Qt SQL, Qt Multimedia.
CMake (можно использовать из Visual Studio).
Python 3:
Скачайте и установите последнюю версию Python с python.org.
Важно: Во время установки поставьте галочку "Add Python to PATH".
2. Установка проекта
Клонирование репозитория (используйте Git Bash или командную строку):
Generated bash
git clone <URL_вашего_репозитория>
cd <имя_папки_проекта>
Use code with caution.
Bash
Настройка сервера (Backend):
Generated bash
# Создаем и активируем виртуальное окружение
python -m venv venv
.\venv\Scripts\activate

# Устанавливаем Python-зависимости
pip install "fastapi[all]" python-jose[cryptography] passlib[bcrypt]
Use code with caution.
Bash
Сборка клиента (Frontend):
Generated powershell
# Создаем директорию для сборки
mkdir build
cd build

# Генерируем сборочные файлы, указав путь к Qt
# Замените путь на ваш реальный путь к Qt
cmake .. -DCMAKE_PREFIX_PATH="C:\Qt\6.5.0\msvc2019_64"

# Собираем проект
cmake --build . --config Release
Use code with caution.
Powershell
После успешной сборки исполняемый файл summerProject.exe появится в директории build\Release.
Установка для Linux (Debian/Ubuntu)
1. Системные требования
Generated bash
# Обновляем список пакетов
sudo apt update

# Устанавливаем инструменты для сборки C++ и CMake
sudo apt install build-essential cmake

# Устанавливаем библиотеки Qt6
sudo apt install qt6-base-dev libqt6sql6-sqlite qt6-multimedia-dev

# Устанавливаем Python и Git
sudo apt install python3 python3-pip python3-venv git
Use code with caution.
Bash
2. Установка проекта
Клонирование репозитория:
Generated bash
git clone <URL_вашего_репозитория>
cd <имя_папки_проекта>
Use code with caution.
Bash
Настройка сервера (Backend):
Generated bash
# Создаем и активируем виртуальное окружение
python3 -m venv venv
source venv/bin/activate

# Устанавливаем Python-зависимости
pip install "fastapi[all]" python-jose[cryptography] passlib[bcrypt]
Use code with caution.
Bash
Сборка клиента (Frontend):
Generated bash
# Создаем директорию для сборки
mkdir build
cd build

# Генерируем сборочные файлы
cmake ..

# Собираем проект, используя все ядра процессора
make -j$(nproc)
Use code with caution.
Bash
После успешной сборки исполняемый файл summerProject появится в директории build.
▶️ Запуск приложения
Для работы приложения необходимо запустить и сервер, и клиент.
Шаг 1: Запуск сервера
Откройте терминал (или cmd/PowerShell на Windows), перейдите в корневую директорию проекта и выполните:
Generated bash
# Активируйте виртуальное окружение, если еще не сделали этого
# Linux:
source venv/bin/activate
# Windows:
.\venv\Scripts\activate

# Запустите сервер
uvicorn server:app --host 0.0.0.0 --port 8000
Use code with caution.
Bash
Сервер будет запущен и готов принимать подключения. Не закрывайте это окно.
Шаг 2: Запуск клиента
Откройте второй терминал (или cmd/PowerShell) и запустите скомпилированное приложение:
Для Linux:
Generated bash
./build/summerProject
Use code with caution.
Bash
Для Windows:
Generated powershell
# Запустите клиент из папки со сборкой
.\build\Release\summerProject.exe
Use code with caution.
Powershell
При первом запуске приложение попросит вас ввести адрес сервера. Используйте:
http://127.0.0.1:8000
После этого вы сможете зарегистрироваться и начать пользоваться ежедневником.
