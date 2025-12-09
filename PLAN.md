# Gazprea Software Engineering Plan

_Last Updated: `November 9, 2025`_ 

Purpose of this document is to outline the software engineering guidelines and practices our team will adhere for the development of Gazprea. This document will cover development workflow, code style convention, CI/CD practices, testing strategy, communication and project management, and plan maintenance.

## 1. Development Workflow

- All development will be done on feature branches created from the `master` branch.
- Each branch is expected to be rebased onto the latest `master` before merging.
- Pull Requests (PRs) must be reviewed and approved by at least one team member before merging.
- PR titles and descriptions should clearly state the purpose and changes made along with an example usage of the new feature or bug fix.
- Only hot fixes can be merged directly into `master` with verbal or written approval of at least two team members.

## 2. Code Style Convention

- Standard code will follow google's style format for C++ code found in this [Clang Format](./.clang-format) with a slight modification to use column length of 100 characters.
- All code upon build will be automatically formatted using clang-format.
- All code is written in CamelCase except [Grammar](./grammar/Gazprea.g4) which is written in snake_case as per ANTLR conventions.
- Comments should be used to explain the purpose of complex code sections, but avoid obvious comments.

## 3. CI/CD Practices

- CI/CD is set up by default using GitHub Actions.
- To save resources, CI pipeline is only triggered on PRs with label `run-ci` and pushes to `master`.
- CI pipeline includes:
  - Building the project using CMake.
  - Running tests using DragonRunner
  - Running memory leak check using Valgrind and DragonRunner.
- Every PR must pass the CI pipeline before it can be merged.

## 4. Testing Strategy

- Tests are written for DragonRunner.
- Each new feature which produces MLIR code must have at least one corresponding test case.
- Bug fixes must include a test case that reproduces the bug.

## 5. Project Management

- Github issues will be used to track tasks and bugs.
- Each PR must reference the issue it addresses.
- Issues will also be put into project boards for better tracking.
- Milestone 1 and Milestone 2 will be used to group issues for part 1 and part 2 of the project respectively.

## 6. Communication

- Team will use discord for daily communication.
- Meetings will be held daily at a time agreed upon by all team members.
- Meeting notes will be converted into issues for better tracking.
- PRs upon completion will be shared in the discord chat for visibility.

## 7. Project Cycle

- Part 1
  - Team will sit down together and read through the spec to understand the requirements.
  - Team will create the grammar together by pair programming.
  - Basic AST structure will be created together.
  - Remaining AST nodes will be created by dividing the work among team members.
  - Entire Def/Ref pass will be created together.
  - Basic validation pass will be created together.
  - Remaining validation pass and MLIR code generation will be divided among team members.
- Part 2
  - Team will sit down together and read through the spec to understand the requirements.
  - Team will create the grammar together by pair programming.
  - Basic refactoring pass will be done together.
  - Team will work on basic validation together.
  - Remaining validation pass and MLIR code generation will be divided among team members.

## 8. Plan Maintenance

This document will be updated as needed throughout the project. Any changes to this plan must be discussed and agreed upon by all team members. Major changes should be documented in the version history section below.

