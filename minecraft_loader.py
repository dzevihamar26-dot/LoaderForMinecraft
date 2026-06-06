#!/usr/bin/env python3
"""
Minecraft Client Loader - Open Source
A cross-platform loader for Minecraft clients built with PyQt6
License: MIT (Free and Open Source)
"""

import sys
import os
import json
import zipfile
import subprocess
import tempfile
from pathlib import Path
from dotenv import load_dotenv

# Load environment variables from .env file
load_dotenv()

from PyQt6.QtWidgets import (
    QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
    QPushButton, QLabel, QProgressBar, QFileDialog, QStackedWidget,
    QComboBox, QMessageBox
)
from PyQt6.QtCore import Qt, QThread, pyqtSignal, QSize
from PyQt6.QtGui import QFont, QIcon

# ============================================================================
# CONFIGURATION - EDIT THESE VALUES IN .env FILE
# ============================================================================

# URL to download the client archive (ZIP file)
CLIENT_DOWNLOAD_URL = os.getenv("CLIENT_DOWNLOAD_URL", "https://example.com/client.zip")

# Name of the JAR file inside the archive (relative path if in subfolder)
CLIENT_JAR_NAME = os.getenv("CLIENT_JAR_NAME", "client.jar")

# Default installation directory (used if user doesn't specify)
DEFAULT_INSTALL_DIR = os.getenv(
    "DEFAULT_INSTALL_DIR",
    "C:/MinecraftClient" if sys.platform == "win32" else "/opt/minecraft-client"
)

# Client display name shown on the main page
CLIENT_DISPLAY_NAME = os.getenv("CLIENT_DISPLAY_NAME", "My Minecraft Client")

# ============================================================================
# TRANSLATIONS
# ============================================================================

TRANSLATIONS = {
    "RU": {
        "select_java_title": "Выбор Java",
        "select_java_label": "Выберите версию Java для запуска:",
        "java_8": "Java 8",
        "java_11": "Java 11",
        "java_17": "Java 17",
        "java_21": "Java 21",
        "select_directory_title": "Выбор директории установки",
        "select_directory_label": "Выберите директорию для установки клиента:",
        "browse": "Обзор...",
        "skip": "Пропустить",
        "next": "Далее",
        "install_location": "Путь установки:",
        "not_selected": "Не выбрано (будет использована директория по умолчанию)",
        "launch": "Запустить",
        "downloading": "Скачивание...",
        "extracting": "Распаковка...",
        "launching": "Запуск...",
        "ready": "Готов к запуску",
        "error": "Ошибка",
        "download_complete": "Загрузка завершена",
        "extract_complete": "Распаковка завершена",
        "select_install_dir": "Выберите директорию для установки",
        "select_java_executable": "Выберите исполняемый файл Java",
        "java_executables": "Java исполняемые файлы",
        "all_files": "Все файлы",
        "language": "Язык",
        "close": "Закрыть",
        "installing": "Установка...",
        "percent_complete": "% завершено"
    },
    "EN": {
        "select_java_title": "Java Selection",
        "select_java_label": "Select Java version to run:",
        "java_8": "Java 8",
        "java_11": "Java 11",
        "java_17": "Java 17",
        "java_21": "Java 21",
        "select_directory_title": "Installation Directory",
        "select_directory_label": "Select directory for client installation:",
        "browse": "Browse...",
        "skip": "Skip",
        "next": "Next",
        "install_location": "Installation path:",
        "not_selected": "Not selected (default directory will be used)",
        "launch": "Launch",
        "downloading": "Downloading...",
        "extracting": "Extracting...",
        "launching": "Launching...",
        "ready": "Ready to launch",
        "error": "Error",
        "download_complete": "Download complete",
        "extract_complete": "Extraction complete",
        "select_install_dir": "Select installation directory",
        "select_java_executable": "Select Java executable",
        "java_executables": "Java executables",
        "all_files": "All files",
        "language": "Language",
        "close": "Close",
        "installing": "Installing...",
        "percent_complete": "% complete"
    }
}


class DownloadWorker(QThread):
    """Worker thread for downloading and extracting the client"""
    progress = pyqtSignal(int)
    status = pyqtSignal(str)
    finished = pyqtSignal(bool, str)
    
    def __init__(self, url, install_dir):
        super().__init__()
        self.url = url
        self.install_dir = install_dir
        
    def run(self):
        try:
            import urllib.request
            
            # Create installation directory
            os.makedirs(self.install_dir, exist_ok=True)
            
            # Download the archive
            self.status.emit("downloading")
            temp_file = os.path.join(tempfile.gettempdir(), "client_download.zip")
            
            def report_hook(block_num, block_size, total_size):
                if total_size > 0:
                    downloaded = block_num * block_size
                    percent = int((downloaded / total_size) * 100)
                    self.progress.emit(min(percent, 99))
            
            urllib.request.urlretrieve(self.url, temp_file, reporthook=report_hook)
            self.progress.emit(50)
            
            # Extract the archive
            self.status.emit("extracting")
            with zipfile.ZipFile(temp_file, 'r') as zip_ref:
                total_files = len(zip_ref.namelist())
                for i, file in enumerate(zip_ref.namelist()):
                    zip_ref.extract(file, self.install_dir)
                    progress = 50 + int((i / total_files) * 49)
                    self.progress.emit(progress)
            
            # Clean up temp file
            os.remove(temp_file)
            
            self.progress.emit(100)
            self.finished.emit(True, "")
            
        except Exception as e:
            self.finished.emit(False, str(e))


class SetupPage(QWidget):
    """Setup wizard page for Java and directory selection"""
    
    def __init__(self, translations, lang="RU"):
        super().__init__()
        self.translations = translations
        self.lang = lang
        self.selected_java = None
        self.selected_dir = None
        self.setup_ui()
        
    def setup_ui(self):
        layout = QVBoxLayout()
        layout.setSpacing(20)
        layout.setContentsMargins(40, 40, 40, 40)
        
        # Title
        title = QLabel(self.translations[self.lang]["select_java_title"])
        title.setFont(QFont("Arial", 18, QFont.Weight.Bold))
        title.setStyleSheet("color: white;")
        title.setAlignment(Qt.AlignmentFlag.AlignCenter)
        layout.addWidget(title)
        
        # Java selection
        java_label = QLabel(self.translations[self.lang]["select_java_label"])
        java_label.setStyleSheet("color: #a0d4a0; font-size: 14px;")
        layout.addWidget(java_label)
        
        self.java_combo = QComboBox()
        self.java_combo.addItems([
            self.translations[self.lang]["java_8"],
            self.translations[self.lang]["java_11"],
            self.translations[self.lang]["java_17"],
            self.translations[self.lang]["java_21"]
        ])
        self.java_combo.setStyleSheet("""
            QComboBox {
                background-color: #1a1a1a;
                color: white;
                border: 2px solid #2d5a2d;
                border-radius: 8px;
                padding: 10px;
                font-size: 14px;
            }
            QComboBox::drop-down {
                border: none;
                width: 30px;
            }
            QComboBox::down-arrow {
                image: none;
                border-left: 5px solid transparent;
                border-right: 5px solid transparent;
                border-top: 8px solid #4a8a4a;
                margin-right: 10px;
            }
            QComboBox QAbstractItemView {
                background-color: #1a1a1a;
                color: white;
                border: 2px solid #2d5a2d;
                selection-background-color: #2d5a2d;
            }
        """)
        layout.addWidget(self.java_combo)
        
        # Directory selection
        dir_label = QLabel(self.translations[self.lang]["select_directory_label"])
        dir_label.setStyleSheet("color: #a0d4a0; font-size: 14px; margin-top: 20px;")
        layout.addWidget(dir_label)
        
        dir_layout = QHBoxLayout()
        
        self.dir_edit = QLabel(self.translations[self.lang]["not_selected"])
        self.dir_edit.setStyleSheet("""
            color: #888;
            background-color: #1a1a1a;
            border: 2px solid #2d5a2d;
            border-radius: 8px;
            padding: 10px;
        """)
        self.dir_edit.setWordWrap(True)
        dir_layout.addWidget(self.dir_edit, 1)
        
        browse_btn = QPushButton(self.translations[self.lang]["browse"])
        browse_btn.setStyleSheet("""
            QPushButton {
                background-color: #2d5a2d;
                color: white;
                border: none;
                border-radius: 8px;
                padding: 10px 20px;
                font-size: 14px;
                font-weight: bold;
            }
            QPushButton:hover {
                background-color: #3d6a3d;
            }
            QPushButton:pressed {
                background-color: #1d4a1d;
            }
        """)
        browse_btn.clicked.connect(self.browse_directory)
        dir_layout.addWidget(browse_btn)
        
        layout.addLayout(dir_layout)
        
        # Buttons
        btn_layout = QHBoxLayout()
        btn_layout.addStretch()
        
        skip_btn = QPushButton(self.translations[self.lang]["skip"])
        skip_btn.setStyleSheet("""
            QPushButton {
                background-color: transparent;
                color: #888;
                border: 2px solid #444;
                border-radius: 8px;
                padding: 12px 30px;
                font-size: 14px;
            }
            QPushButton:hover {
                border-color: #666;
                color: #aaa;
            }
        """)
        skip_btn.clicked.connect(self.skip_setup)
        btn_layout.addWidget(skip_btn)
        
        next_btn = QPushButton(self.translations[self.lang]["next"])
        next_btn.setStyleSheet("""
            QPushButton {
                background-color: #2d5a2d;
                color: white;
                border: none;
                border-radius: 8px;
                padding: 12px 40px;
                font-size: 14px;
                font-weight: bold;
            }
            QPushButton:hover {
                background-color: #3d6a3d;
            }
            QPushButton:pressed {
                background-color: #1d4a1d;
            }
        """)
        next_btn.clicked.connect(self.complete_setup)
        btn_layout.addWidget(next_btn)
        
        layout.addLayout(btn_layout)
        layout.addStretch()
        
        self.setLayout(layout)
    
    def browse_directory(self):
        directory = QFileDialog.getExistingDirectory(
            self,
            self.translations[self.lang]["select_install_dir"],
            "",
            QFileDialog.Option.ShowDirsOnly
        )
        if directory:
            self.selected_dir = directory
            self.dir_edit.setText(directory)
            self.dir_edit.setStyleSheet("""
                color: white;
                background-color: #1a1a1a;
                border: 2px solid #2d5a2d;
                border-radius: 8px;
                padding: 10px;
            """)
    
    def skip_setup(self):
        self.selected_java = self.java_combo.currentText()
        self.parent().stacked_widget.setCurrentIndex(1)
    
    def complete_setup(self):
        self.selected_java = self.java_combo.currentText()
        if not self.selected_dir:
            self.selected_dir = DEFAULT_INSTALL_DIR
        self.parent().stacked_widget.setCurrentIndex(1)


class MainPage(QWidget):
    """Main page with launch button"""
    
    def __init__(self, translations, lang="RU"):
        super().__init__()
        self.translations = translations
        self.lang = lang
        self.download_worker = None
        self.setup_ui()
        
    def setup_ui(self):
        layout = QVBoxLayout()
        layout.setSpacing(15)
        layout.setContentsMargins(40, 40, 40, 40)
        
        # Client name
        self.name_label = QLabel(CLIENT_DISPLAY_NAME)
        self.name_label.setFont(QFont("Arial", 24, QFont.Weight.Bold))
        self.name_label.setStyleSheet("color: white;")
        self.name_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        layout.addWidget(self.name_label)
        
        layout.addStretch()
        
        # Launch button (rounded)
        self.launch_btn = QPushButton(self.translations[self.lang]["launch"])
        self.launch_btn.setMinimumSize(200, 60)
        self.launch_btn.setMaximumSize(300, 80)
        self.launch_btn.setStyleSheet("""
            QPushButton {
                background-color: #2d5a2d;
                color: white;
                border: none;
                border-radius: 30px;
                font-size: 18px;
                font-weight: bold;
            }
            QPushButton:hover {
                background-color: #3d6a3d;
            }
            QPushButton:pressed {
                background-color: #1d4a1d;
            }
            QPushButton:disabled {
                background-color: #1a3a1a;
                color: #666;
            }
        """)
        self.launch_btn.clicked.connect(self.launch_client)
        
        btn_layout = QHBoxLayout()
        btn_layout.addStretch()
        btn_layout.addWidget(self.launch_btn)
        btn_layout.addStretch()
        layout.addLayout(btn_layout)
        
        # Status bar (progress bar)
        self.status_label = QLabel(self.translations[self.lang]["ready"])
        self.status_label.setStyleSheet("color: #a0d4a0; font-size: 14px;")
        self.status_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        layout.addWidget(self.status_label)
        
        self.progress_bar = QProgressBar()
        self.progress_bar.setVisible(False)
        self.progress_bar.setStyleSheet("""
            QProgressBar {
                background-color: #1a1a1a;
                border: 2px solid #2d5a2d;
                border-radius: 10px;
                text-align: center;
                color: white;
                height: 20px;
            }
            QProgressBar::chunk {
                background-color: #2d5a2d;
                border-radius: 8px;
            }
        """)
        layout.addWidget(self.progress_bar)
        
        layout.addStretch()
        
        self.setLayout(layout)
    
    def launch_client(self):
        """Handle launch button click"""
        self.launch_btn.setEnabled(False)
        self.progress_bar.setVisible(True)
        self.progress_bar.setValue(0)
        
        install_dir = self.parent().setup_page.selected_dir or DEFAULT_INSTALL_DIR
        jar_path = os.path.join(install_dir, CLIENT_JAR_NAME)
        
        # Check if client is already installed
        if os.path.exists(jar_path):
            self.run_client(jar_path)
        else:
            # Download and extract
            self.status_label.setText(self.translations[self.lang]["downloading"])
            self.download_worker = DownloadWorker(CLIENT_DOWNLOAD_URL, install_dir)
            self.download_worker.progress.connect(self.update_progress)
            self.download_worker.status.connect(self.update_status)
            self.download_worker.finished.connect(self.on_download_finished)
            self.download_worker.start()
    
    def update_progress(self, value):
        self.progress_bar.setValue(value)
    
    def update_status(self, status_key):
        self.status_label.setText(self.translations[self.lang].get(status_key, status_key))
    
    def on_download_finished(self, success, error_msg):
        if success:
            self.status_label.setText(self.translations[self.lang]["download_complete"])
            install_dir = self.parent().setup_page.selected_dir or DEFAULT_INSTALL_DIR
            jar_path = os.path.join(install_dir, CLIENT_JAR_NAME)
            self.run_client(jar_path)
        else:
            self.status_label.setText(f"{self.translations[self.lang]['error']}: {error_msg}")
            self.launch_btn.setEnabled(True)
            QMessageBox.critical(self, self.translations[self.lang]["error"], error_msg)
    
    def run_client(self, jar_path):
        """Launch the Minecraft client JAR"""
        self.status_label.setText(self.translations[self.lang]["launching"])
        self.progress_bar.setValue(100)
        
        try:
            java_cmd = "java"
            if self.parent().setup_page.selected_java:
                # Map Java version to command (in real implementation, you'd find actual Java paths)
                version_map = {
                    "Java 8": "java",
                    "Java 11": "java",
                    "Java 17": "java",
                    "Java 21": "java"
                }
                java_cmd = version_map.get(self.parent().setup_page.selected_java, "java")
            
            # Launch the JAR
            subprocess.Popen([java_cmd, "-jar", jar_path], cwd=os.path.dirname(jar_path))
            
            self.status_label.setText(self.translations[self.lang]["ready"])
            self.launch_btn.setEnabled(True)
            
        except Exception as e:
            self.status_label.setText(f"{self.translations[self.lang]['error']}: {str(e)}")
            self.launch_btn.setEnabled(True)
            QMessageBox.critical(self, self.translations[self.lang]["error"], str(e))


class MainWindow(QMainWindow):
    """Main application window"""
    
    def __init__(self):
        super().__init__()
        self.current_lang = "RU"
        self.setWindowTitle(CLIENT_DISPLAY_NAME)
        self.setMinimumSize(600, 500)
        self.setStyleSheet("background-color: #0a0a0a;")
        
        self.setup_ui()
        
    def setup_ui(self):
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        
        main_layout = QVBoxLayout()
        main_layout.setContentsMargins(0, 0, 0, 0)
        central_widget.setLayout(main_layout)
        
        # Top bar with language selector
        top_bar = QHBoxLayout()
        top_bar.setContentsMargins(20, 15, 20, 15)
        
        top_bar.addStretch()
        
        # Language selector
        lang_label = QLabel("Language: ")
        lang_label.setStyleSheet("color: #888;")
        top_bar.addWidget(lang_label)
        
        self.lang_combo = QComboBox()
        self.lang_combo.addItems(["RU", "EN"])
        self.lang_combo.setCurrentText("RU")
        self.lang_combo.setStyleSheet("""
            QComboBox {
                background-color: #1a1a1a;
                color: white;
                border: 2px solid #2d5a2d;
                border-radius: 6px;
                padding: 5px 10px;
            }
            QComboBox::drop-down {
                border: none;
                width: 20px;
            }
            QComboBox QAbstractItemView {
                background-color: #1a1a1a;
                color: white;
                border: 2px solid #2d5a2d;
            }
        """)
        self.lang_combo.currentTextChanged.connect(self.change_language)
        top_bar.addWidget(self.lang_combo)
        
        main_layout.addLayout(top_bar)
        
        # Stacked widget for pages
        self.stacked_widget = QStackedWidget()
        self.stacked_widget.setStyleSheet("background-color: #0a0a0a;")
        
        # Setup page
        self.setup_page = SetupPage(TRANSLATIONS, self.current_lang)
        self.setup_page.parent = lambda: self
        self.stacked_widget.addWidget(self.setup_page)
        
        # Main page
        self.main_page = MainPage(TRANSLATIONS, self.current_lang)
        self.main_page.parent = lambda: self
        self.stacked_widget.addWidget(self.main_page)
        
        main_layout.addWidget(self.stacked_widget)
    
    def change_language(self, lang):
        self.current_lang = lang
        self.setup_page.lang = lang
        self.main_page.lang = lang
        
        # Update all UI text
        self.setup_page.setup_ui()
        self.main_page.setup_ui()


def main():
    app = QApplication(sys.argv)
    
    # Set application style
    app.setStyle("Fusion")
    
    window = MainWindow()
    window.show()
    
    sys.exit(app.exec())


if __name__ == "__main__":
    main()
