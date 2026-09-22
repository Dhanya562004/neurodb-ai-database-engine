# 🚀 NeuroDB – AI-Powered Hybrid Database Engine

## 📌 Overview
NeuroDB is a **hybrid AI-powered database engine** that combines a **C++ query processor**, a **Python fallback execution layer**, and a **modern Streamlit web interface**.

It allows users to query structured data using:
- Natural Language (AI)
- SQL Queries

The system intelligently switches to a **pandas-based fallback engine** when the C++ backend is unavailable (e.g., Streamlit Cloud), ensuring **cross-platform reliability**.

---

## 🌐 Live Demo
👉 https://neurodb-ai-database-engine-eernxxm8yetvbdyhoyoazw.streamlit.app/

---

## ✨ Key Features

- 🤖 Natural Language → SQL (AI-powered)
- 🧠 Intelligent Query Translation (LLM + rule-based fallback)
- ⚡ Hybrid Execution Engine:
  - C++ Core (local high-performance execution)
  - Python Fallback (cloud-compatible execution)
- 📊 CSV-based Storage System
- 📋 Interactive Streamlit UI
- 📈 Query Performance Metrics (latency, rows, status)
- 📜 Query Logging & Audit Trail
- 🗂️ Schema Explorer Dashboard
- ☁️ Fully Functional on Streamlit Cloud

---

## 🏗️ Architecture

```
User Input (NL / SQL)
        ↓
AI Translator (Gemini / Rule-based)
        ↓
Query Validator
        ↓
Execution Layer
   ├── C++ Engine (Local)
   └── Python Fallback (Cloud)
        ↓
Results → Streamlit UI
```

---

## 🛠️ Tech Stack

- **C++** – Core database engine
- **Python** – Wrapper + fallback execution
- **Streamlit** – Frontend UI
- **Pandas** – Query execution (fallback mode)
- **Gemini API / Rule-based parser** – AI translation
- **Git** – Version control

---

## 📂 Project Structure

```
NeuroDB/
│
├── src/                # C++ source code
├── include/            # C++ headers
├── frontend/           # Streamlit app
├── python_wrapper/     # Python connector & AI modules
├── data/               # CSV datasets
├── logs/               # Query logs
├── tests/              # Unit tests
├── scripts/            # Build scripts
├── neurodb.exe         # Compiled C++ engine
├── requirements.txt
└── README.md
```

---

## ▶️ Run Locally

### 1. Clone Repository
```
git clone https://github.com/Dhanya562004/neurodb-ai-database-engine.git
cd neurodb-ai-database-engine
```

### 2. Create Virtual Environment
```
python -m venv venv
venv\Scripts\activate
```

### 3. Install Dependencies
```
pip install -r requirements.txt
```

### 4. Run Application
```
streamlit run frontend/app.py
```

---

## 🧪 Sample Queries

### Natural Language
```
show students with marks greater than 80
```

### SQL
```
SELECT * FROM students;
SELECT * FROM products WHERE price < 100;
```

---

## ⚠️ Important Note

Due to platform limitations (e.g., Streamlit Cloud), the **C++ engine may not execute in cloud environments**.

To ensure availability, NeuroDB includes a **Python fallback execution engine** using pandas, allowing queries to run seamlessly without native binaries.

---

## 📈 Resume Highlight

Built a hybrid AI-powered database engine combining C++ core processing with Python fallback execution, enabling natural language querying and cloud deployment via Streamlit.

---

## 📬 Author

**Dhanya K**  
B.Tech AIML (2026)  
GitHub: https://github.com/Dhanya562004  
