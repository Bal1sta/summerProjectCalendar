from fastapi import FastAPI, HTTPException
from pydantic import BaseModel
import sqlite3
from typing import List


app = FastAPI()
DB_PATH = "notes.db"


# Создаем таблицу, если не существует
with sqlite3.connect(DB_PATH) as conn:
    conn.execute('''CREATE TABLE IF NOT EXISTS notes (
        date TEXT,
        hour INTEGER,
        minute INTEGER,
        note TEXT,
        PRIMARY KEY(date, hour, minute)
    )''')


class Note(BaseModel):
    date: str   # формат ISO, например "2025-07-24"
    hour: int
    minute: int
    note: str


@app.get("/")
def read_root():
    return {"status": "Notes server is running :)"}


@app.get("/dates_with_notes", response_model=List[str])
def get_dates_with_notes():
    with sqlite3.connect(DB_PATH) as conn:
        rows = conn.execute("SELECT DISTINCT date FROM notes").fetchall()
    return [row[0] for row in rows]


@app.get("/notes/{date}")
def get_notes_for_date(date: str):
    with sqlite3.connect(DB_PATH) as conn:
        rows = conn.execute(
            "SELECT hour, minute, note FROM notes WHERE date = ?", (date,)
        ).fetchall()
    return [{"hour": r[0], "minute": r[1], "note": r[2]} for r in rows]


@app.post("/save_note")
def save_note(note: Note):
    with sqlite3.connect(DB_PATH) as conn:
        conn.execute(
            "INSERT OR REPLACE INTO notes (date, hour, minute, note) VALUES (?, ?, ?, ?)",
            (note.date, note.hour, note.minute, note.note)
        )
    return {"status": "ok"}


@app.post("/delete_note")
def delete_note(note: Note):
    with sqlite3.connect(DB_PATH) as conn:
        cur = conn.execute(
            "DELETE FROM notes WHERE date = ? AND hour = ? AND minute = ?",
            (note.date, note.hour, note.minute)
        )
    if cur.rowcount == 0:
        raise HTTPException(status_code=404, detail="Заметка не найдена")
    return {"status": "ok"}


# uvicorn имя_вашего_файла:app --host 0.0.0.0 --port 8000
# uvicorn server:app --host 0.0.0.0 --port 8000
# uvicorn server:app --host 25.46.140.115 --port 8000


# http://localhost:8000 (если запуск локальный)

# или http://<IP_вашего_Hamachi>:8000 (если хотите подключаться через Hamachi из другой машины)

# http://25.46.186.188:8000/