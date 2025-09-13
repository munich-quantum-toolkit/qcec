<!--- This file has been generated from an external template. Please do not modify it directly. -->
<!--- Changes should be contributed to https://github.com/munich-quantum-toolkit/templates. -->

# Installation

MQT QCEC is primarily developed as a C++20 library with Python bindings.
The Python package is available on [PyPI](https://pypi.org/project/mqt.qcec/) and can be installed on all major operating systems with all [officially supported Python versions](https://devguide.python.org/versions/).

:::::{tip}
:name: uv-recommendation

We recommend using [{code}`uv`][uv].
It is a fast Python package and project manager by [Astral](https://astral.sh/) (creators of [{code}`ruff`][ruff]).
It can replace {code}`pip` and {code}`virtualenv`, automatically manages virtual environments, installs packages, and can install Python itself.
It is significantly faster than {code}`pip`.

If you do not have {code}`uv` installed, install it with:

::::{tab-set}
:::{tab-item} macOS and Linux

```console
$ curl -LsSf https://astral.sh/uv/install.sh | sh
```

:::
:::{tab-item} Windows

```console
$ powershell -ExecutionPolicy ByPass -c "irm https://astral.sh/uv/install.ps1 | iex"
```

:::
::::

See the [uv documentation][uv] for more information.

:::::

::::{tab-set}
:sync-group: installer

:::{tab-item} {code}`uv` _(recommended)_
:sync: uv

```console
$ uv pip install mqt.qcec
```

:::

:::{tab-item} {code}`pip`
:sync: pip

```console
(.venv) $ python -m pip install mqt.qcec
```

:::
::::
In most cases, no compilation is required; a platform-specific prebuilt wheel is downloaded and installed.

Verify the installation:

```console
(.venv) $ python -c "import mqt.qcec; print(mqt.qcec.__version__)"
```

This prints the installed package version.

## Building from Source for Performance

To get the best performance and enable platform-specific optimizations not available in portable wheels, we recommend building the library from source:

::::{tab-set}
:sync-group: installer

:::{tab-item} {code}`uv` _(recommended)_
:sync: uv

```console
$ uv pip install mqt.qcec --no-binary mqt.qcec
```

:::

:::{tab-item} {code}`pip`
:sync: pip

```console
(.venv) $ pip install mqt.qcec --no-binary mqt.qcec
```

:::
::::
This requires a C++20-capable [C++ compiler](https://en.wikipedia.org/wiki/List_of_compilers#C++_compilers) and [CMake](https://cmake.org/) 3.24 or newer.

## Integrating MQT QCEC into Your Project

To use the MQT QCEC Python package in your project, add it as a dependency in your {code}`pyproject.toml` or {code}`setup.py`.
This ensures the package is installed when your project is installed.

::::{tab-set}

:::{tab-item} {code}`uv` _(recommended)_

```console
$ uv add mqt.qcec
```

:::

:::{tab-item} {code}`pyproject.toml`

```toml
[project]
# ...
dependencies = ["mqt.qcec>=<version>"]
# ...
```

:::

:::{tab-item} {code}`setup.py`

```python
from setuptools import setup

setup(
    # ...
    install_requires=["mqt.qcec>=<version>"],
    # ...
)
```

:::
::::

If you want to integrate the C++ library directly into your project, you can either

- add it as a [{code}`git` submodule][git-submodule] and build it as part of your project, or
- install MQT QCEC on your system and use CMake's {code}`find_package()` command to locate it, or
- use CMake's [{code}`FetchContent`][FetchContent] module to combine both approaches.

::::{tab-set}
:::{tab-item} {code}`FetchContent`

This is the recommended approach because it lets you detect installed versions of MQT QCEC and only downloads the library if it is not available on the system.
Furthermore, CMake's [{code}`FetchContent`][FetchContent] module provides flexibility in how the library is integrated into the project.

```cmake
include(FetchContent)
set(FETCH_PACKAGES "")

# cmake-format: off
set(MQT_QCEC_MINIMUM_VERSION "<minimum_version>"
    CACHE STRING "MQT QCEC minimum version")
set(MQT_QCEC_VERSION "<version>"
    CACHE STRING "MQT QCEC version")
set(MQT_QCEC_REV "<revision>"
    CACHE STRING "MQT QCEC identifier (tag, branch or commit hash)")
set(MQT_QCEC_REPO_OWNER "munich-quantum-toolkit"
    CACHE STRING "MQT QCEC repository owner (change when using a fork)")
# cmake-format: on
FetchContent_Declare(
  mqt-qcec
  GIT_REPOSITORY https://github.com/${MQT_QCEC_REPO_OWNER}/qcec.git
  GIT_TAG ${MQT_QCEC_REV}
  FIND_PACKAGE_ARGS ${MQT_QCEC_MINIMUM_VERSION})
list(APPEND FETCH_PACKAGES mqt-qcec)

# Make all declared dependencies available.
FetchContent_MakeAvailable(${FETCH_PACKAGES})
```

:::

:::{tab-item} {code}`git-submodule`

Adding the library as a [{code}`git` submodule][git-submodule] is a simple approach.
However, {code}`git` submodules can be cumbersome, especially when working with multiple branches or versions of the library.
First, add the submodule to your project (e.g., in the {code}`external` directory):

```console
$ git submodule add https://github.com/munich-quantum-toolkit/qcec.git external/mqt-qcec
```

Then add the following line to your {code}`CMakeLists.txt` to make the library's targets available in your project:

```cmake
add_subdirectory(external/mqt-qcec)
```

:::

:::{tab-item} {code}`find_package()`

You can install MQT QCEC on your system after building it from source:

```console
$ git clone https://github.com/munich-quantum-toolkit/qcec.git mqt-qcec
$ cd mqt-qcec
$ cmake -S . -B build
$ cmake --build build
$ cmake --install build
```

Then, in your project's {code}`CMakeLists.txt`, use {code}`find_package()` to locate the installed library:

```cmake
find_package(mqt-qcec <version> REQUIRED)
```

:::

::::

(development-setup)=

## Development Setup

Set up a reproducible development environment for MQT QCEC.
This is the recommended starting point for both bug fixes and new features.
For detailed guidelines and workflows, see {doc}`contributing`.

1.  Get the code:

    ::::{tab-set}
    :::{tab-item} External Contribution
    If you do not have write access to the [munich-quantum-toolkit/qcec](https://github.com/munich-quantum-toolkit/qcec) repository, fork the repository on GitHub (see <https://docs.github.com/en/get-started/quickstart/fork-a-repo>) and clone your fork locally.

    ```console
    $ git clone git@github.com:your_name_here/qcec.git mqt-qcec
    ```

    :::
    :::{tab-item} Internal Contribution
    If you have write access to the [munich-quantum-toolkit/qcec](https://github.com/munich-quantum-toolkit/qcec) repository, clone the repository locally.

    ```console
    $ git clone git@github.com/munich-quantum-toolkit/qcec.git mqt-qcec
    ```

    :::
    ::::

2.  Change into the project directory:

    ```console
    $ cd mqt-qcec
    ```

3.  Create a branch for local development:

    ```console
    $ git checkout -b name-of-your-bugfix-or-feature
    ```

    Now you can make your changes locally.

4.  Install development tools:

    We highly recommend using modern, fast tooling for the development workflow.
    We recommend using [{code}`uv`][uv].
    If you don't have {code}`uv`, follow the installation instructions in the recommendation above (see {ref}`tip above <uv-recommendation>`).
    See the [uv documentation][uv] for more information.

    We also recommend installing [{code}`pre-commit`][pre-commit] to automatically run checks before each commit and [{code}`nox`][nox] to automate common development tasks.

    ::::{tab-set}
    :sync-group: installer

    :::{tab-item} {code}`uv` _(recommended)_
    :sync: uv
    The easiest way to install {code}`pre-commit` and {code}`nox` is via [{code}`uv`][uv]:

    ```console
    $ uv tool install pre-commit
    $ uv tool install nox
    ```

    :::
    :::{tab-item} {code}`brew`
    :sync: brew
    On macOS with Homebrew, you can install {code}`pre-commit` and {code}`nox` with:

    ```console
    $ brew install pre-commit nox
    ```

    :::
    :::{tab-item} {code}`pipx`
    :sync: pipx
    If you prefer to use [{code}`pipx`][pipx], you can install {code}`pre-commit` and {code}`nox` with:

    ```console
    $ pipx install pre-commit
    $ pipx install nox
    ```

    :::
    :::{tab-item} {code}`pip`
    :sync: pip
    If you prefer to use regular {code}`pip` (preferably in a virtual environment), you can install {code}`pre-commit` and {code}`nox` with:

    ```console
    $ pip install pre-commit nox
    ```

    :::
    ::::

    Then enable the {code}`pre-commit` hooks with:

    ```console
    $ pre-commit install
    ```

<!-- Links -->

[FetchContent]: https://cmake.org/cmake/help/latest/module/FetchContent.html
[git-submodule]: https://git-scm.com/docs/git-submodule
[nox]: https://nox.thea.codes/en/stable/
[pipx]: https://pypa.github.io/pipx/
[pre-commit]: https://pre-commit.com/
[ruff]: https://docs.astral.sh/ruff/
[uv]: https://docs.astral.sh/uv/
