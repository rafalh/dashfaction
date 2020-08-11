#!/usr/bin/env python3
import sys

def process_patch(patch_file_name, input_file_name, output_file_name):
    with open(input_file_name, "rb") as f:
        buf = f.read()
    print('Input file size:', len(buf))
    with open(patch_file_name) as f:
        for line in f:
            line = line.strip()
            # skip comments
            if not line or line.startswith('#'):
                continue
            tokens = line.split(' ')
            print(line)
            op = tokens[0].lower()
            if op == 'mov':
                src_start = int(tokens[1], 0)
                src_end_inclusive = int(tokens[2], 0)
                src_end = src_end_inclusive + 1 # exclusive end
                num_bytes = src_end - src_start
                assert num_bytes > 0
                tgt_start = int(tokens[3], 0)
                if tgt_start >= src_end:
                    buf = buf[:src_start] + buf[src_end:tgt_start] + buf[src_start:src_end] + buf[tgt_start:]
                elif tgt_start <= src_start:
                    buf = buf[:tgt_start] + buf[src_start:src_end] + buf[tgt_start:src_start] + buf[src_end:]
                else:
                    assert False
            elif op == 'rem':
                start = int(tokens[1], 0)
                end_inclusive = int(tokens[2], 0)
                end = end_inclusive + 1
                num_bytes = end - start
                assert num_bytes > 0
                buf = buf[:start] + buf[end:]
            elif op == 'ins':
                tgt_start = int(tokens[1], 0)
                data = bytes.fromhex(tokens[2])
                buf = buf[:tgt_start] + data + buf[tgt_start:]
            elif op == 'rep':
                tgt_start = int(tokens[1], 0)
                data = bytes.fromhex(tokens[2])
                tgt_end = tgt_start + len(data)
                buf = buf[:tgt_start] + data + buf[tgt_end:]
            else:
                assert False
    print('Output file size:', len(buf))
    with open(output_file_name, "wb") as f:
        f.write(buf)

def main(argv):
    if len(argv) < 4:
        print('Usage: %s patch_file input_file output_file')
        return
    patch_file_name = argv[1]
    input_file_name = argv[2]
    output_file_name = argv[3]
    process_patch(patch_file_name, input_file_name, output_file_name)

if __name__ == "__main__":
    main(sys.argv)
