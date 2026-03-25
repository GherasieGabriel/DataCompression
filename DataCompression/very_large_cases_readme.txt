Very Large Test Cases for Timing

1) large_case_01_repetitive.txt
   - Size: 100000 characters
   - Pattern: highly repetitive ("ABCD" repeated)
   - Expected: strong compression, faster match reuse

2) large_case_02_random.txt
   - Size: 100000 characters
   - Pattern: random alphanumeric
   - Expected: weak compression, worst/near-worst matching behavior

3) large_case_03_natural.txt
   - Size: 116200 characters
   - Pattern: natural-language-like repeated phrase
   - Expected: medium-to-high compression

4) large_case_04_mixed_blocks.txt
   - Size: 107200 characters
   - Pattern: mixed repetitive and short variant blocks
   - Expected: medium compression, varied behavior

How to use:
- Copy one file's contents into test_data.txt, then run compression.
- Record the printed input size and compression/decompression times.
