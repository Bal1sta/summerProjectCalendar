# from fastapi import FastAPI, HTTPException
# from pydantic import BaseModel
# import sqlite3
# from typing import List


# app = FastAPI()
# DB_PATH = "notes.db"


# # Создаем таблицу, если не существует
# with sqlite3.connect(DB_PATH) as conn:
#     conn.execute('''CREATE TABLE IF NOT EXISTS notes (
#         date TEXT,
#         hour INTEGER,
#         minute INTEGER,
#         note TEXT,
#         PRIMARY KEY(date, hour, minute)
#     )''')


# class Note(BaseModel):
#     date: str   # формат ISO, например "2025-07-24"
#     hour: int
#     minute: int
#     note: str


# @app.get("/")
# def read_root():
#     return {"status": "Notes server is running :)"}


# @app.get("/dates_with_notes", response_model=List[str])
# def get_dates_with_notes():
#     with sqlite3.connect(DB_PATH) as conn:
#         rows = conn.execute("SELECT DISTINCT date FROM notes").fetchall()
#     return [row[0] for row in rows]


# @app.get("/notes/{date}")
# def get_notes_for_date(date: str):
#     with sqlite3.connect(DB_PATH) as conn:
#         rows = conn.execute(
#             "SELECT hour, minute, note FROM notes WHERE date = ?", (date,)
#         ).fetchall()
#     return [{"hour": r[0], "minute": r[1], "note": r[2]} for r in rows]


# @app.post("/save_note")
# def save_note(note: Note):
#     with sqlite3.connect(DB_PATH) as conn:
#         conn.execute(
#             "INSERT OR REPLACE INTO notes (date, hour, minute, note) VALUES (?, ?, ?, ?)",
#             (note.date, note.hour, note.minute, note.note)
#         )
#     return {"status": "ok"}


# @app.post("/delete_note")
# def delete_note(note: Note):
#     with sqlite3.connect(DB_PATH) as conn:
#         cur = conn.execute(
#             "DELETE FROM notes WHERE date = ? AND hour = ? AND minute = ?",
#             (note.date, note.hour, note.minute)
#         )
#     if cur.rowcount == 0:
#         raise HTTPException(status_code=404, detail="Заметка не найдена")
#     return {"status": "ok"}


# # uvicorn имя_вашего_файла:app --host 0.0.0.0 --port 8000
# # uvicorn server:app --host 0.0.0.0 --port 8000
# # uvicorn server:app --host 25.46.140.115 --port 8000


# # http://localhost:8000 (если запуск локальный)

# # или http://<IP_вашего_Hamachi>:8000 (если хотите подключаться через Hamachi из другой машины)

# # http://25.46.186.188:8000/




from fastapi import FastAPI, HTTPException, Depends, status
from fastapi.security import OAuth2PasswordBearer, OAuth2PasswordRequestForm
from pydantic import BaseModel
from typing import Optional, List
import sqlite3
from passlib.context import CryptContext
from jose import JWTError, jwt
from datetime import datetime, timedelta

# --- Конфигурация ---
APP_SECRET = "CHANGE_THIS_SECRET"  # Замените на свой сложный секретный ключ
ALGORITHM = "HS256"
ACCESS_TOKEN_EXPIRE_MINUTES = 30

DB_PATH = "notes_shared.db"

# --- Инициализация FastAPI ---
app = FastAPI()

# --- Инициализация базы данных ---
def init_db():
    with sqlite3.connect(DB_PATH) as conn:
        # Таблица пользователей
        conn.execute("""
            CREATE TABLE IF NOT EXISTS users (
                username TEXT PRIMARY KEY,
                hashed_password TEXT NOT NULL
            )
        """)
        # Таблица заметок
        conn.execute("""
            CREATE TABLE IF NOT EXISTS notes (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                owner TEXT,
                date TEXT,
                hour INTEGER,
                minute INTEGER,
                note TEXT,
                shared_with TEXT,
                FOREIGN KEY(owner) REFERENCES users(username)
            )
        """)

init_db()

# --- Password hashing и OAuth2 ---
# pwd_context = CryptContext(schemes=["argon2"], deprecated="auto")
pwd_context = CryptContext(schemes=["bcrypt"], deprecated="auto")
oauth2_scheme = OAuth2PasswordBearer(tokenUrl="/token")

def verify_password(plain_password, hashed_password):
    return pwd_context.verify(plain_password, hashed_password)

def get_password_hash(password):
    return pwd_context.hash(password)

# --- Работа с пользователями в базе ---
def get_user(username: str):
    with sqlite3.connect(DB_PATH) as conn:
        cur = conn.execute("SELECT username, hashed_password FROM users WHERE username = ?", (username,))
        row = cur.fetchone()
        if row:
            return {"username": row[0], "hashed_password": row[1]}
    return None

def authenticate_user(username: str, password: str):
    user = get_user(username)
    if not user:
        return False
    if not verify_password(password, user["hashed_password"]):
        return False
    return user

def create_access_token(data: dict, expires_delta: Optional[timedelta] = None):
    to_encode = data.copy()
    expire = datetime.utcnow() + (expires_delta or timedelta(minutes=15))
    to_encode.update({"exp": expire})
    return jwt.encode(to_encode, APP_SECRET, algorithm=ALGORITHM)

async def get_current_user(token: str = Depends(oauth2_scheme)):
    credentials_exception = HTTPException(
        status_code=status.HTTP_401_UNAUTHORIZED,
        detail="Неверные учетные данные",
        headers={"WWW-Authenticate": "Bearer"},
    )
    try:
        payload = jwt.decode(token, APP_SECRET, algorithms=[ALGORITHM])
        username: str = payload.get("sub")
        if username is None:
            raise credentials_exception
    except JWTError:
        raise credentials_exception
    user = get_user(username)
    if user is None:
        raise credentials_exception
    return user

# --- Pydantic модели ---
class UserCreate(BaseModel):
    username: str
    password: str

class Note(BaseModel):
    id: Optional[int] = None
    date: str
    hour: int
    minute: int
    note: str
    shared_with: Optional[List[str]] = []

# --- Эндпоинты ---

# Регистрация пользователя
@app.post("/register", status_code=201)
def register(user: UserCreate):
    if get_user(user.username):
        raise HTTPException(status_code=400, detail="Пользователь уже существует")
    hashed_password = get_password_hash(user.password)
    with sqlite3.connect(DB_PATH) as conn:
        conn.execute("INSERT INTO users (username, hashed_password) VALUES (?, ?)", (user.username, hashed_password))
    return {"msg": "Пользователь создан"}

# Вход с выдачей токена
@app.post("/token")
def login(form_data: OAuth2PasswordRequestForm = Depends()):
    user = authenticate_user(form_data.username, form_data.password)
    if not user:
        raise HTTPException(status_code=401, detail="Неверный логин или пароль")
    access_token = create_access_token(
        data={"sub": user["username"]},
        expires_delta=timedelta(minutes=ACCESS_TOKEN_EXPIRE_MINUTES)
    )
    return {"access_token": access_token, "token_type": "bearer"}

# Пример защищённого эндпоинта для проверки текущего пользователя
@app.get("/me")
def read_users_me(current_user=Depends(get_current_user)):
    return {"username": current_user["username"]}

# Получить список заметок текущего пользователя (включая те, которыми с ним поделились)
@app.get("/notes", response_model=List[Note])
def get_notes(current_user=Depends(get_current_user)):
    username = current_user["username"]
    with sqlite3.connect(DB_PATH) as conn:
        rows = conn.execute(
            """SELECT id, date, hour, minute, note, shared_with FROM notes 
               WHERE owner = ? OR (shared_with LIKE '%' || ? || '%')""",
            (username, username)
        ).fetchall()
    result = []
    for r in rows:
        shared = r[5].split(",") if r[5] else []
        result.append(Note(id=r[0], date=r[1], hour=r[2], minute=r[3], note=r[4], shared_with=shared))
    return result

# Добавить новую заметку
@app.post("/notes", response_model=Note)
def add_note(note: Note, current_user=Depends(get_current_user)):
    username = current_user["username"]
    shared_str = ",".join(note.shared_with) if note.shared_with else ""
    with sqlite3.connect(DB_PATH) as conn:
        cur = conn.execute(
            "INSERT INTO notes (owner, date, hour, minute, note, shared_with) VALUES (?, ?, ?, ?, ?, ?)",
            (username, note.date, note.hour, note.minute, note.note, shared_str)
        )
        note.id = cur.lastrowid
    return note

# Обновить существующую заметку (только если владелец текущий пользователь)
@app.put("/notes/{note_id}", response_model=Note)
def update_note(note_id: int, note: Note, current_user=Depends(get_current_user)):
    username = current_user["username"]
    with sqlite3.connect(DB_PATH) as conn:
        cur = conn.execute("SELECT owner FROM notes WHERE id = ?", (note_id,))
        row = cur.fetchone()
        if not row or row[0] != username:
            raise HTTPException(status_code=403, detail="Нет прав для редактирования")
        shared_str = ",".join(note.shared_with) if note.shared_with else ""
        conn.execute(
            "UPDATE notes SET date=?, hour=?, minute=?, note=?, shared_with=? WHERE id=?",
            (note.date, note.hour, note.minute, note.note, shared_str, note_id)
        )
    note.id = note_id
    return note

# Удалить заметку (только если владелец)
@app.delete("/notes/{note_id}")
def delete_note(note_id: int, current_user=Depends(get_current_user)):
    username = current_user["username"]
    with sqlite3.connect(DB_PATH) as conn:
        cur = conn.execute("SELECT owner FROM notes WHERE id = ?", (note_id,))
        row = cur.fetchone()
        if not row or row[0] != username:
            raise HTTPException(status_code=403, detail="Нет прав для удаления")
        conn.execute("DELETE FROM notes WHERE id = ?", (note_id,))
    return {"msg": "Заметка удалена"}

# --- Запуск через uvicorn ---
if __name__ == "__main__":
    import uvicorn
    uvicorn.run("main:app", host="0.0.0.0", port=8000, reload=True)

@app.get("/")
def root():
    return {"message": "API работает"}


{
    "username": "testuser",
    "password": "testpassword"
}

# # Функция создания пользователя по умолчанию
# def create_default_user():
#     if not get_user("qwe"):
#         hashed_password = get_password_hash("123")
#         with sqlite3.connect(DB_PATH) as conn:
#             conn.execute(
#                 "INSERT INTO users (username, hashed_password) VALUES (?, ?)",
#                 ("qwe", hashed_password)
#             )
#         print('Пользователь "qwe" с паролем "123" создан автоматически.')


# # Инициализация БД и создание дефолтного пользователя
# init_db()
# create_default_user()
