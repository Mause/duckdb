import * as JSZip from "jszip";
import {context,} from '@actions/github';
import {writeFile} from "fs/promises";

const DESIRED = ['OSX', 'Windows', 'LinuxRelease'];
const flatten = (arr) => [].concat(...arr);

async function downloadJar(github, artifact) {
    console.log('downloading', artifact.name);
    let download = await github.rest.actions.downloadArtifact({
        owner: context.repo.owner,
        repo: context.repo.repo,
        artifact_id: artifact.id,
        archive_format: 'zip',
    });
    const jars = (await JSZip.loadAsync(download.data)).file(/duckdb_jdbc.jar/);
    return await Promise.all(jars.map(jar => JSZip.loadAsync(jar.async('uint8array'))));
}

async function downloadJarsFromWorkflow(github, workflow_run) {
    const allArtifacts = await github.rest.actions.listWorkflowRunArtifacts({
        owner: context.repo.owner,
        repo: context.repo.repo,
        run_id: workflow_run.id,
    });
    return flatten(await Promise.all(allArtifacts.data.artifacts
        .filter(artifact => artifact.name.startsWith("duckdb-binaries-"))
        .map(artifact => downloadJar(github, artifact))));
}

async function mergeJars(jars) {
    const [dest, ...src] = jars;
    for (const jar of src) {
        const lib = jar.file(/\.so/)[0];
        console.log('adding', lib.name);
        dest.file(lib.name, lib.nodeStream());
    }
    return dest;
}

module.exports = async function ({ github }) {
    const allRuns = await github.rest.actions.listWorkflowRunsForRepo({
        owner: context.repo.owner,
        repo: context.repo.repo,
        branch: context.payload.workflow_run.head_branch,
        status: 'success',
        per_page: 200
    });
    const workflows = allRuns.data.workflow_runs.filter(wr => wr.head_sha === context.payload.workflow_run.head_sha);

    const names = new Set(workflows.map(workflow => workflow.name));
    const missing = DESIRED.filter(name => !names.has(name));
    if (missing.length) {
        github.log.warn('Not ready yet, missing', missing);
        return;
    } else {
        console.log('We have everything we need');
    }

    const jars = flatten(await Promise.all(workflows.map(workflow_run => downloadJarsFromWorkflow(github, workflow_run))));
    console.log('combining', jars.length, 'jars');

    const jar = await mergeJars(jars);
    await writeFile('duckdb_jdbc_all.jar', await jar.generateAsync({ type: 'nodebuffer' }));
};
