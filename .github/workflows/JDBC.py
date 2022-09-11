import json
import logging
import os
import shutil
from concurrent.futures import ThreadPoolExecutor
from io import BytesIO
from itertools import chain
from typing import Dict, List, IO, Generator
from zipfile import ZipFile

import requests
from octokit import Octokit

executor = ThreadPoolExecutor(10)

logging.basicConfig(level=logging.INFO)

DESIRED = ['OSX', 'Windows', 'LinuxRelease']
FILENAME = 'duckdb_jdbc_all.jar'
flatten = chain.from_iterable


def download_jar(github: Octokit, context: Dict, artifact: Dict) -> Generator[IO[bytes], None, None]:
    owner = context['repo']['owner']
    repo = context['repo']['repo']

    print('downloading', artifact['name'])
    download = requests.get(
        f'https://api.github.com/repos/{owner}/{repo}/actions/artifacts/{artifact["id"]}/zip',
        **github._auth({'headers': {}})
    )
    zip_file = ZipFile(BytesIO(download.content))
    print('downloaded', artifact['name'])
    return (zip_file.open(file) for file in zip_file.filelist if file.filename.endswith("duckdb_jdbc.jar"))


def download_jars_from_workflow(github: Octokit, context: Dict, workflow_run) -> chain[IO[bytes]]:
    all_artifacts = github.actions.list_workflow_run_artifacts(
        owner=context['repo']['owner'],
        repo=context['repo']['repo'],
        run_id=workflow_run['id'],
    )
    return flatten(
        download_jar(github, context, artifact)
        for artifact in all_artifacts.json['artifacts']
        if artifact['name'].startswith("duckdb-binaries-")
    )


def merge_jars(jars: List[IO[bytes]]) -> None:
    dest, *src = jars

    print(f'using {dest} as base jar')

    with open(FILENAME, 'wb') as fh:
        shutil.copyfileobj(dest, fh)

    with ZipFile(FILENAME, 'a') as dest:
        for jar in src:
            zip_file = ZipFile(jar)
            lib = next(f for f in zip_file.filelist if ".so" in f.filename)
            print('adding', lib.filename)
            with dest.open(lib.filename, 'w') as fh:
                shutil.copyfileobj(zip_file.open(lib), fh)


def main(github: Octokit, context: Dict) -> None:
    workflow_run = context['payload']['workflow_run']
    repo = context['repo']['repo']
    owner = context['repo']['owner']
    head_sha = workflow_run['head_sha']

    print(
        f'checking for workflow runs against {head_sha} see '
        f'https://github.com/{owner}/{repo}/commit/{head_sha}/status-details for full details',
    )
    print('triggered by "{name}" with conclusion "{conclusion}", see {html_url} for more details'.format_map(workflow_run))

    all_runs = github.actions.list_workflow_runs_for_repo(
        owner=owner,
        repo=repo,
        branch=workflow_run['head_branch'],
        status='success',
        per_page=200
    )
    workflows = [wr for wr in all_runs.json['workflow_runs'] if wr['head_sha'] == head_sha]

    names = {wr['name'] for wr in workflows}
    missing = {name for name in DESIRED if name not in names}
    if missing:
        print('Not ready yet, waiting on', missing)
        return
    else:
        print('We have everything we need')

    jars = list(
        flatten(executor.map(lambda workflow: download_jars_from_workflow(github, context, workflow), workflows)))
    print('combining', len(jars), 'jars')

    merge_jars(jars)


def run():
    github = Octokit(auth='token', token=os.environ['GITHUB_TOKEN'])

    with open(os.environ['GITHUB_EVENT_PATH']) as fh:
        payload = json.load(fh)

    owner, repo = os.environ['GITHUB_REPOSITORY'].split('/')

    main(
        github=github,
        context={
            "payload": payload,
            "repo": {
                'owner': owner,
                'repo': repo
            }
        }
    )


if __name__ == '__main__':
    run()
