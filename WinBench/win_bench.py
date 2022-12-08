import os
import shutil
import itertools
import time
import glob


def update_files():
    print("Updating files")

    # Copy executable
    shutil.copy("../Debug/NRC-HPM-Renderer.exe", ".")

    # Copy dll files
    dll_files = glob.iglob(os.path.join("../Debug/", "*.dll"))
    for dll_file in dll_files:
        if os.path.isfile(dll_file):
            shutil.copy2(dll_file, ".")

    # Copy data dir
    shutil.rmtree("./data")
    shutil.copytree("../data", "./data")

    # Copy imgui file
    shutil.copy("../imgui.ini", ".")

    # Create output directory
    if not os.path.exists("output"):
        os.makedirs("output")


def generate_configs():
    print("Generating app configs for NRC-HPM-Renderer.exe")

    loss_fn_options = ["RelativeL2"]
    optimizer_options = ["Adam"]
    learning_rate_options = ["0.01", "0.001", "0.0001"]
    encoding_options = ["0"]
    nn_width_options = ["64", "128"]
    nn_depth_options = ["2", "6", "10"]
    log2_batch_size_options = ["12", "14", "16"]
    scene_options = ["0"]
    render_width_options = ["1920"]
    render_height_options = ["1080"]
    train_sample_ratio_options = ["0.01", "0.02", "0.05", "0.1"]
    train_spp_options = ["1", "2", "4"]
    
    arguments_list = list(itertools.product(
        loss_fn_options, 
        optimizer_options, 
        learning_rate_options, 
        encoding_options, 
        nn_width_options, 
        nn_depth_options, 
        log2_batch_size_options, 
        scene_options,
        render_width_options,
        render_height_options,
        train_sample_ratio_options,
        train_spp_options))
    arguments_list_len = len(arguments_list)
    print(str(arguments_list_len) + " configs created")
    
    configs_file = open("configs.csv", "w")

    for index, arguments in enumerate(arguments_list):
        arguments_str = ""
        for arg in arguments:
            arguments_str += str(arg) + " "
        arguments_list[index] = arguments_str

        configs_file.write(arguments_str + "\n")
        #print(str(index) + ": " + str(arguments_str))

    configs_file.close()


def execute_config(index, arguments):
    print("Executing config " + str(index) + ": " + str(arguments))
    
    start_time = time.time()
    os.system("NRC-HPM-Renderer " + arguments)
    end_time = time.time()
    print("Execution took " + str(end_time - start_time) + " seconds")


def execute_configs():
    print("Executing app configs")

    config_file = open("configs.csv", "r")
    arguments_list = config_file.readlines()
    config_file.close()

    print(str(len(arguments_list)) + " configs loaded")

    for index, arguments in enumerate(arguments_list):
        arguments = arguments.strip()
        arguments_list[index] = arguments
        execute_config(index, arguments)


def evaluate_results():
    print("Evaluating results")


def main():
    print("Starting NRC-HPM-Bench")
    update_files()
    #generate_configs()
    #execute_configs()
    #evaluate_results()


if __name__ == "__main__":
    main()
