Import("env")
import shutil
import os

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

