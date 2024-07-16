import os
import json


def walk_directory_and_collect_textures(directory_path, file_extension):
    texture_files = []
    for _, _, files in os.walk(directory_path):
        for file in files:
            if file.endswith(file_extension):
                texture_files.append(file)
    return texture_files


def save_textures_to_json(texture_files, output_path):
    with open(output_path, "w") as json_file:
        json.dump(texture_files, json_file, indent=4)


def set_block_texture_json():
    directory_path = "resources/data/block/model"  # Specify your directory path here
    output_path = "resources/data/block/block_model_data.json"
    texture_files = walk_directory_and_collect_textures(directory_path, ".json")
    save_textures_to_json(texture_files, output_path)


def set_block_model_texture_json():
    directory_path = "resources/textures/block"  # Specify your directory path here
    output_path = "resources/data/block/textures.json"
    texture_files = walk_directory_and_collect_textures(directory_path, ".png")
    save_textures_to_json(texture_files, output_path)


def main():
    set_block_texture_json()


if __name__ == "__main__":
    main()
