Import("env")
import shutil
import os
import hashlib
import subprocess
import datetime

def generate_hash():
    # Ścieżki do katalogów z kodem źródłowym
    source_dirs = ['src','flash_image']

    hash_md5 = hashlib.md5()
    for source_dir in source_dirs:
        for root, dirs, files in os.walk(source_dir):
            for file in sorted(files):
                file_path = os.path.join(root, file)
                with open(file_path, 'rb') as f:
                    # Aktualizuj hash na podstawie zawartości każdego pliku
                    while chunk := f.read(8192):
                        hash_md5.update(chunk)

    return hash_md5.hexdigest()

def get_git_commit():
    try:
        # Pobierz hash aktualnego commita
        commit_hash = subprocess.check_output(['git', 'rev-parse', 'HEAD']).strip().decode('utf-8')
        return commit_hash
    except subprocess.CalledProcessError:
        return "unknown"  # W przypadku błędu lub braku repozytorium
    
def sign_release():

    current_hash = generate_hash()
    current_commit = get_git_commit()

    # Wczytaj ostatni zapisany hash i commit
    last_hash_file = "last_build_hash.txt"
    if os.path.exists(last_hash_file):
        with open(last_hash_file, "r") as f:
            last_hash = f.read().strip()
    else:
        last_hash = ""

    # Porównaj hash obecnego stanu kodu z ostatnim hashem
    if current_hash != last_hash:
        # Hash się zmienił, aktualizujemy build info
        build_time = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        with open("include/build_info.h", "w") as f:
            f.write(f'#define BUILD_TIME "{build_time}"\n')
            f.write(f'#define GIT_COMMIT "{current_commit}"\n')

        # Zapisz nowy hash
        with open(last_hash_file, "w") as f:
            f.write(current_hash)
    else:
        # Hash się nie zmienił, nie aktualizujemy build info
        print("No changes detected. Build number and commit remain the same.")




def before_build(source, target, env):
    print("Copying data from flash_image to data")
    file_list = os.listdir("flash_image")
    if os.path.exists("data"): shutil.rmtree("data")
    os.mkdir("data")
    for x in file_list:
        if x != "config-example.cfg":
            shutil.copy("flash_image/" + x,"data")


print("Current CLI targets", COMMAND_LINE_TARGETS)
print("Current Build targets", BUILD_TARGETS)
env.AddPreAction("$BUILD_DIR/littlefs.bin", before_build)

sign_release()

