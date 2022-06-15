import argparse
import os
from typing import List


def main(path: str,
         files: List[str],
         side: str,
         cycle: int,
         out_path: str,
         out_name: str,
         max_goals_in_file: int,
         goals_limit: int):
    saved_file_number = 0
    goal_count = 0
    out_lines = []
    processed_files_count = 0
    print(files)
    for f in files:
        processed_files_count += 1
        lines = open(os.path.join(path, f), 'r').readlines()
        start_line = -1
        goal_lines_number = []
        for l in range(len(lines)):
            if lines[l].startswith('(show 1 '):
                start_line = l
            # if lines[l].startswith('(playmode'):
            #     print(lines[l])
            if lines[l].startswith('(playmode') and lines[l].strip().endswith(f'goal_{side})'):
                goal_lines_number.append(l)
        print(f, start_line)
        print(goal_lines_number)
        for goal_line_number in goal_lines_number:
            start = max(goal_line_number - cycle, start_line)
            for i in range(start, goal_line_number + 1):
                out_lines.append(lines[i])
            goal_count += 1

        done = False
        if goal_count > goals_limit:
            done = True
        if goal_count > max_goals_in_file or processed_files_count == len(files):
            goal_count = 0
            out_file_name = os.path.join(out_path, out_name + f'_{saved_file_number}' + '.rcg')
            out_file = open(out_file_name, 'w')
            out_file.write('ULG5\n')
            out_file.writelines(out_lines)
            out_file.close()
            saved_file_number += 1
        if done:
            break


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Extract goals from rcg.')
    parser.add_argument('--path', '-p', type=str,
                        help='path directory', required=True)
    parser.add_argument('--file', '-f', type=str, default='',
                        help='file name')
    parser.add_argument('--side', '-s', type=str, default='l',
                        help='[l,r,b] l=left team goals, r=right team goals')
    parser.add_argument('--cycle', '-c', type=int, default=50, help='number of cycles before')
    parser.add_argument('--out_path', '-o', type=str, default='./tmp', help='output should be save here')
    parser.add_argument('--out_name', '-n', type=str, default='out', help='output file name without .rcg')
    parser.add_argument('--max_goals', '-m', type=int, default=100, help='max goals in a file!')
    parser.add_argument('--goals', '-g', type=int, default=100, help='max goals in a file!')
    args = parser.parse_args()

    path: str = args.path
    files: List[str] = []
    if len(args.file) > 0:
        if not args.file.endswith('.rcg'):
            raise Exception('file name should end with .rcg')
        if not os.path.isfile(os.path.join(path, args.file)):
            raise Exception('the rcg file is not exist')
        files.append(args.file)
    else:
        if not os.path.exists(path):
            raise Exception('path directory is not exist')
        all_files = os.listdir(path)
        for file in all_files:
            if file.endswith('.rcg'):
                files.append(file)
    if len(files) == 0:
        raise Exception('there is not any rcg file')
    side = args.side
    cycle = args.cycle
    out_path = args.out_path
    if not os.path.exists(out_path):
        raise Exception('out path is not exist')
    out_name: str = args.out_name
    if out_name.endswith('.rcg'):
        out_name = out_name.replace('.rcg', '')
    max_goals_in_file = args.max_goals
    goals_limit = args.goals
    print(f'''
                path = {path}
                len(files) = {len(files)}
                side = {side}
                cycle = {cycle}
                out_path = {out_path}
                out_name = {out_name}
                max_goals_in_file = {max_goals_in_file}
                goals_limit = {goals_limit}
            ''')
    main(
        path,
        files,
        side,
        cycle,
        out_path,
        out_name,
        max_goals_in_file,
        goals_limit,
    )