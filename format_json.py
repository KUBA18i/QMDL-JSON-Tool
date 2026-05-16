import json
import re
import sys
import os

def flatten(json_obj):
    standard_json = json.dumps(json_obj, indent=2)
    def collapse(match):
        content = re.sub(r'\s+', ' ', match.group(0))
        return content.replace('[ ', '[').replace(' ]', ']')

    pattern = r'\[[\d\s.,\[\]\-a-zA-Z]*\]'
    flattened = re.sub(pattern, collapse, standard_json)
    return flattened

def main():
    if len(sys.argv) < 2:
        print("Usage: python format_json.py <path_to_json_file>")
        return

    file_path = sys.argv[1]
    
    if not os.path.exists(file_path):
        print(f"Error: File '{file_path}' not found.")
        return

    try:
        with open(file_path, 'r') as f:
            data = json.load(f)

        final_output = flatten(data)

        with open(file_path, 'w') as f:
            f.write(final_output)
            
        print(f"Success! {file_path} has been flattened.")

    except Exception as e:
        print(f"An error occurred: {e}")

if __name__ == "__main__":
    main()