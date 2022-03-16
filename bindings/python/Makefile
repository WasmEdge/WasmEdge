.ONESHELL:
ENV_PREFIX=$(shell python -c "if __import__('pathlib').Path('.venv/bin/pip').exists(): print('.venv/bin/')")

.PHONY: help
help:             ## Show the help.
	@echo "Usage: make <target>"
	@echo ""
	@echo "Targets:"
	@fgrep "##" Makefile | fgrep -v fgrep


.PHONY: show
show:             ## Show the current environment.
	@echo "Current environment:"
	@echo "Running using $(ENV_PREFIX)"
	@$(ENV_PREFIX)python -V
	@$(ENV_PREFIX)python -m site

.PHONY: install
install:          ## Install the project in dev mode.
	@echo "Don't forget to run 'make virtualenv' if you got errors."
	$(ENV_PREFIX)pip install -e .[test]

.PHONY: stubgen
stubgen:          ## Generate stub using stubgen
	$(ENV_PREFIX)stubgen -m WasmEdge --doc-dir docs_sphinx/_generate -o pywasmedge/stubs -v

.PHONY: pypi-test
pypi-test:          ## PyPi testing
	$(ENV_PREFIX)pip install build twine
	$(ENV_PREFIX)python -m build --sdist
	$(ENV_PREFIX)twine check dist/* && $(ENV_PREFIX)twine upload --repository testpypi dist/* --verbose

.PHONY: fmt
fmt:              ## Format code using black & isort.
	$(ENV_PREFIX)isort pywasmedge/
	$(ENV_PREFIX)black -l 79 pywasmedge/
	$(ENV_PREFIX)black -l 79 tests/
	@find . -regex '.*\.\(cpp\|hpp\|cc\|cxx\)' -not -path "./pybind11/*" -exec clang-format -style=file -i {} \;

.PHONY: lint
lint:             ## Run pep8, black, mypy linters.
	$(ENV_PREFIX)flake8 pywasmedge/
	$(ENV_PREFIX)black -l 79 --check pywasmedge/
	$(ENV_PREFIX)black -l 79 --check tests/
	$(ENV_PREFIX)mypy --ignore-missing-imports pywasmedge/

.PHONY: test
test: lint        ## Run tests and generate coverage report.
	$(ENV_PREFIX)pytest -v --cov-config .coveragerc --cov=pywasmedge -l --tb=short tests/
	$(ENV_PREFIX)coverage xml
	$(ENV_PREFIX)coverage html

.PHONY: watch
watch:            ## Run tests on every change.
	ls **/**.py | entr $(ENV_PREFIX)pytest -s -vvv -l --tb=long --maxfail=1 tests/

.PHONY: clean
clean:            ## Clean unused files.
	@find ./ -name '*.pyc' -exec rm -f {} \;
	@find ./ -name '__pycache__' -exec rm -rf {} \;
	@find ./ -name 'Thumbs.db' -exec rm -f {} \;
	@find ./ -name '*~' -exec rm -f {} \;
	@rm -rf .cache
	@rm -rf .pytest_cache
	@rm -rf .mypy_cache
	@rm -rf build
	@rm -rf dist
	@rm -rf *.egg-info
	@rm -rf htmlcov
	@rm -rf .tox/
	@rm -rf docs/_build

.PHONY: virtualenv
virtualenv:       ## Create a virtual environment.
	@echo "creating virtualenv ..."
	@rm -rf .venv
	@python3 -m venv .venv
	@./.venv/bin/pip install -U pip
	@./.venv/bin/pip install -e .[test]
	@echo
	@echo "!!! Please run 'source .venv/bin/activate' to enable the environment !!!"

.PHONY: release
release:          ## Create a new tag for release.
	@echo "WARNING: This operation will create s version tag and push to github"
	@read -p "Version? (provide the next x.y.z semver) : " TAG
	@echo "creating git tag : $${TAG}"
	@git tag $${TAG}
	@echo "$${TAG}" > pywasmedge/VERSION
	@$(ENV_PREFIX)gitchangelog HEAD...origin/master > docs/HISTORY.md
	@git add pywasmedge/VERSION docs/HISTORY.md
	@git commit -m "release: version $${TAG} 🚀"
	@git push -u origin HEAD --tags
	@echo "Github Actions will detect the new tag and release the new version."

.PHONY: docs
docs:             ## Build the documentation.
	@echo "building documentation ..."
	@$(ENV_PREFIX)gitchangelog HEAD...origin/master > docs/HISTORY.md
	@echo '```' > docs/commands/auto_generated.md
	@/usr/bin/pydoc3 WasmEdge >> docs/commands/auto_generated.md
	@git add docs/commands/auto_generated.md
	@echo '```' >> docs/commands/auto_generated.md
	@$(ENV_PREFIX)mkdocs build
	@$(ENV_PREFIX)mkdocs serve

.PHONY: deploy-gh-pages
deploy-gh-pages:
	@$(ENV_PREFIX)gitchangelog HEAD...origin/master > docs/HISTORY.md
	@echo '```' > docs/commands/auto_generated.md
	@/usr/bin/pydoc3 WasmEdge >> docs/commands/auto_generated.md
	@echo '```' >> docs/commands/auto_generated.md
	@$(ENV_PREFIX)mkdocs gh-deploy --config-file mkdocs.yml --remote-branch "gh-pages" --remote-name "satacker_r"

