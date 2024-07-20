import os
import json
from typing import List


def read_json(path):
    try:
        with open(path, "r") as file:
            data = json.load(file)
            return data
    except FileNotFoundError:
        print(f"Error: The file '{path}' was not found.")
        return None
    except json.JSONDecodeError:
        print("Error: Failed to parse JSON.")
        return None


def write_json(obj, output_path):
    with open(output_path, "w") as json_file:
        json.dump(obj, json_file, indent=2)


def walk_directory_and_collect_textures(directory_path, file_extension):
    texture_files = []
    for _, _, files in os.walk(directory_path):
        for file in files:
            if file.endswith(file_extension):
                texture_files.append(file)
    return texture_files


def set_block_texture_json():
    directory_path = "resources/data/block/model"  # Specify your directory path here
    output_path = "resources/data/block/block_model_data.json"
    texture_files = walk_directory_and_collect_textures(directory_path, ".json")
    write_json(texture_files, output_path)


def write_2d_array(
    texture_paths, output_path, width, height, filter_min="linear", filter_max="linear"
):
    obj = {}
    obj["width"] = width
    obj["height"] = height
    obj["textures"] = texture_paths
    obj["filter_min"] = filter_min
    obj["filter_max"] = filter_max
    obj["path"] = 
    write_json(obj, output_path)


def generate_block_texture_array(model_names):
    model_names = [s.rsplit("/", 1)[-1] for s in model_names]
    textures = set()
    for model in model_names:
        j = read_json(f"resources/data/block/model/{model}.json")
        for texVals in j["textures"].values():
            textures.add(f"{texVals}.png")
    write_2d_array(
        list(textures),
        "resources/data/block/texture_2d_array.json",
        16,
        16,
        "nearest",
        "nearest",
    )


def used_tex_models_from_block_data(path) -> List[str]:
    data = read_json(path)
    used = set()
    for i in range(len(data)):
        used.add(data[i]["model"])
    return list(used)


def main():
    set_block_texture_json()
    used = used_tex_models_from_block_data("resources/data/block/block_data.json")
    generate_block_texture_array(used)
    used = [f'{val.rsplit("/", 1)[-1]}.json' for val in used]
    write_json(used, "resources/data/block/block_model_data.json")


if __name__ == "__main__":
    main()
