from fastapi import FastAPI, HTTPException, Depends, status
from fastapi.security import OAuth2PasswordBearer, OAuth2PasswordRequestForm
from pydantic import BaseModel
from typing import Optional, List
import sqlite3
from passlib.context import CryptContext
from jose import JWTError, jwt
from datetime import datetime, timedelta

# --- Конфигурация ---
APP_SECRET = "YOUR_SUPER_SECRET_KEY_HERE"  # Замените на свой сложный ключ
ALGORITHM = "HS256"
ACCESS_TOKEN_EXPIRE_MINUTES = 30
DB_PATH = "notes_shared.db"

# --- Предопределенные роли ----------------------------------------------
PREDEFINED_ROLES = {
    "qwe": "editor",
    "ewq": "editor"
#    "qwe": "viewer"

}
DEFAULT_ROLE = "viewer"
# ------------------------------------------------------------------------


# --- Инициализация FastAPI ---
app = FastAPI()

# --- Инициализация базы данных (убрали колонку role) ---
def init_db():
    with sqlite3.connect(DB_PATH) as conn:
        conn.execute("""
            CREATE TABLE IF NOT EXISTS users (
                username TEXT PRIMARY KEY,
                hashed_password TEXT NOT NULL
            )
        """)
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

# --- Утилиты для паролей и токенов (без изменений) ---
pwd_context = CryptContext(schemes=["bcrypt"], deprecated="auto")
oauth2_scheme = OAuth2PasswordBearer(tokenUrl="/token")

def verify_password(plain_password, hashed_password):
    return pwd_context.verify(plain_password, hashed_password)

def get_password_hash(password):
    return pwd_context.hash(password)

def create_access_token(data: dict, expires_delta: Optional[timedelta] = None):
    to_encode = data.copy()
    expire = datetime.utcnow() + (expires_delta or timedelta(minutes=ACCESS_TOKEN_EXPIRE_MINUTES))
    to_encode.update({"exp": expire})
    return jwt.encode(to_encode, APP_SECRET, algorithm=ALGORITHM)

# --- Аутентификация и авторизация ---
def get_user_from_db(username: str):
    with sqlite3.connect(DB_PATH) as conn:
        conn.row_factory = sqlite3.Row
        cur = conn.execute("SELECT username, hashed_password FROM users WHERE username = ?", (username,))
        row = cur.fetchone()
        if row:
            return dict(row)
    return None

def authenticate_user(username: str, password: str):
    user = get_user_from_db(username)
    if not user:
        return False
    if not verify_password(password, user["hashed_password"]):
        return False
    return user

async def get_current_user(token: str = Depends(oauth2_scheme)):
    credentials_exception = HTTPException(
        status_code=status.HTTP_401_UNAUTHORIZED,
        detail="Неверные учетные данные", headers={"WWW-Authenticate": "Bearer"},
    )
    try:
        payload = jwt.decode(token, APP_SECRET, algorithms=[ALGORITHM])
        username: str = payload.get("sub")
        role: str = payload.get("role") # Извлекаем роль из токена
        if username is None or role is None:
            raise credentials_exception
        token_data = {"username": username, "role": role}
    except JWTError:
        raise credentials_exception
    return token_data

async def get_current_editor(current_user: dict = Depends(get_current_user)):
    if current_user.get("role") != "editor":
        raise HTTPException(
            status_code=status.HTTP_403_FORBIDDEN,
            detail="Недостаточно прав для выполнения этого действия",
        )
    return current_user


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
@app.post("/register", status_code=201)
def register(user: UserCreate):
    if get_user_from_db(user.username):
        raise HTTPException(status_code=400, detail="Пользователь уже существует")
    hashed_password = get_password_hash(user.password)
    with sqlite3.connect(DB_PATH) as conn:
        conn.execute("INSERT INTO users (username, hashed_password) VALUES (?, ?)", (user.username, hashed_password))
    return {"msg": "Пользователь создан"}

@app.post("/token")
def login(form_data: OAuth2PasswordRequestForm = Depends()):
    user = authenticate_user(form_data.username, form_data.password)
    if not user:
        raise HTTPException(status_code=401, detail="Неверный логин или пароль")

    # Определяем роль на основе имени пользователя
    user_role = PREDEFINED_ROLES.get(user["username"], DEFAULT_ROLE)

    access_token = create_access_token(
        data={"sub": user["username"], "role": user_role}, # Записываем роль в токен
        expires_delta=timedelta(minutes=ACCESS_TOKEN_EXPIRE_MINUTES)
    )
    return {"access_token": access_token, "token_type": "bearer", "user_role": user_role}

@app.get("/notes", response_model=List[Note])
def get_notes(current_user: dict = Depends(get_current_user)):

    with sqlite3.connect(DB_PATH) as conn:
        conn.row_factory = sqlite3.Row
        rows = conn.execute(
            """SELECT id, date, hour, minute, note, shared_with FROM notes"""
        ).fetchall()
        
    result = []
    for r in rows:
        # Преобразование строки 'shared_with' в список остается, на случай если поле используется где-то еще
        shared = r["shared_with"].split(",") if r["shared_with"] else []
        result.append(Note(id=r["id"], date=r["date"], hour=r["hour"], minute=r["minute"], note=r["note"], shared_with=shared))
        
    return result

@app.post("/notes", response_model=Note)
def add_note(note: Note, current_user: dict = Depends(get_current_editor)): # Только для editor
    # ... код без изменений ...
    username = current_user["username"]
    shared_str = ",".join(note.shared_with) if note.shared_with else ""
    with sqlite3.connect(DB_PATH) as conn:
        cur = conn.execute(
            "INSERT INTO notes (owner, date, hour, minute, note, shared_with) VALUES (?, ?, ?, ?, ?, ?)",
            (username, note.date, note.hour, note.minute, note.note, shared_str)
        )
        note.id = cur.lastrowid
    return note

@app.put("/notes/{note_id}", response_model=Note)
def update_note(note_id: int, note: Note, current_user: dict = Depends(get_current_editor)):
    # username = current_user["username"] # Эта строка больше не нужна

    with sqlite3.connect(DB_PATH) as conn:
        # Проверяем, что заметка просто существует
        cur = conn.execute("SELECT owner FROM notes WHERE id = ?", (note_id,))
        row = cur.fetchone()
        if not row:
            raise HTTPException(status_code=404, detail="Заметка не найдена")
        shared_str = ",".join(note.shared_with) if note.shared_with else ""
        conn.execute(
            "UPDATE notes SET date=?, hour=?, minute=?, note=?, shared_with=? WHERE id=?",
            (note.date, note.hour, note.minute, note.note, shared_str, note_id)
        )
    note.id = note_id
    return note


@app.delete("/notes/{note_id}")
def delete_note(note_id: int, current_user: dict = Depends(get_current_editor)):
    # username = current_user["username"] # И эта строка больше не нужна

    with sqlite3.connect(DB_PATH) as conn:
        # Проверяем, что заметка просто существует
        cur = conn.execute("SELECT owner FROM notes WHERE id = ?", (note_id,))
        row = cur.fetchone()
        if not row:
            raise HTTPException(status_code=404, detail="Заметка не найдена")
        conn.execute("DELETE FROM notes WHERE id = ?", (note_id,))
    return {"msg": "Заметка удалена"}