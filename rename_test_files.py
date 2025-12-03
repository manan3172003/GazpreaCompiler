#!/usr/bin/env python3
"""
Script to rename test files based on their directory structure.
Files in subdirectories will be renamed to include the directory path as a prefix.

Example: decs/array/1.in -> decs-array-1.in
"""

import os
import sys
from pathlib import Path


def rename_files(base_dir, dry_run=True):
    """
    Rename files in the directory tree to include their directory path as a prefix.

    Args:
        base_dir: Base directory to process

        dry_run: If True, only print what would be done without actually renaming
    """
    base_path = Path(base_dir)

    if not base_path.exists():
        print(f"Error: Directory {base_dir} does not exist")
        return

    # Collect all files to rename
    files_to_rename = []

    for root, dirs, files in os.walk(base_path):
        root_path = Path(root)

        # Skip the base directory itself
        if root_path == base_path:
            continue

        for filename in files:
            file_path = root_path / filename

            # Get the relative path from base directory
            rel_path = file_path.relative_to(base_path)

            # Get directory components (excluding the filename)
            dir_parts = rel_path.parent.parts

            # Create new filename: join directory parts with hyphens, then add original filename
            if dir_parts:
                new_filename = "-".join(dir_parts) + "-" + filename
            else:
                new_filename = filename

            # New path will be in the same directory
            new_path = root_path / new_filename

            if file_path != new_path:
                files_to_rename.append((file_path, new_path))

    if not files_to_rename:
        print("No files to rename.")
        return

    # Display changes
    print(f"{'DRY RUN - ' if dry_run else ''}Found {len(files_to_rename)} files to rename:\n")

    for old_path, new_path in files_to_rename:
        rel_old = old_path.relative_to(base_path)
        rel_new = new_path.relative_to(base_path)
        print(f"  {rel_old} -> {rel_new}")

    if dry_run:
        print("\n" + "="*60)
        print("This was a DRY RUN. No files were actually renamed.")
        print("Run with --execute to perform the actual renaming.")
        return

    # Perform actual renaming
    print("\n" + "="*60)
    print("Renaming files...\n")

    for old_path, new_path in files_to_rename:
        try:
            old_path.rename(new_path)
            print(f"✓ Renamed: {old_path.name} -> {new_path.name}")
        except Exception as e:
            print(f"✗ Error renaming {old_path}: {e}")

    print(f"\nCompleted! Renamed {len(files_to_rename)} files.")


def main():
    if len(sys.argv) < 2:
        print("Usage: python rename_test_files.py <directory> [--execute]")
        print("\nBy default, runs in dry-run mode (no actual changes).")
        print("Add --execute flag to perform actual renaming.")
        sys.exit(1)

    directory = sys.argv[1]
    execute = "--execute" in sys.argv

    rename_files(directory, dry_run=not execute)


if __name__ == "__main__":
    main()