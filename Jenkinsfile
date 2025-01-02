pipeline {
    agent any
    
    parameters {
        string(name: 'BRANCH', defaultValue: 'main', description: 'Branch to build')
        string(name: 'GITHUB_REPO_URL', defaultValue: 'https://github.com/agustingroh/jenkins-test-personal/', description: 'GitHub repository URL')
        string(name: 'SCANOSS_API_TOKEN_ID', defaultValue:"scanoss-token", description: 'The reference ID for the SCANOSS API TOKEN credential')
        string(name: 'SCANOSS_SBOM_IDENTIFY', defaultValue:"sbom.json", description: 'SCANOSS SBOM Identify filename')
        string(name: 'SCANOSS_SBOM_IGNORE', defaultValue:"sbom-ignore.json", description: 'SCANOSS SBOM Ignore filename')
        string(name: 'SCANOSS_CLI_DOCKER_IMAGE', defaultValue:"ghcr.io/scanoss/scanoss-py:v1.9.0", description: 'SCANOSS CLI Docker Image')
        booleanParam(name: 'ENABLE_DELTA_ANALYSIS', defaultValue: false, description: 'Analyze those files what have changed or new ones')
        string(name: 'JIRA_TOKEN_ID', defaultValue:"jira-token" , description: 'Jira token id')
        string(name: 'JIRA_URL', defaultValue:"https://scanoss.atlassian.net/" , description: 'Jira URL')
        string(name: 'JIRA_PROJECT_KEY', defaultValue:"TESTPROJ" , description: 'Jira Project Key')
        booleanParam(name: 'CREATE_JIRA_ISSUE', defaultValue: false, description: 'Enable Jira reporting')
        booleanParam(name: 'ABORT_ON_POLICY_FAILURE', defaultValue: false, description: 'Abort Pipeline on pipeline Failure')
    }
    
    environment {
        SCANOSS_BUILD_BASE_PATH = "scanoss/${BUILD_NUMBER}"
        SCANOSS_REPORT_FOLDER_PATH = "${SCANOSS_BUILD_BASE_PATH}/reports"
    }
    
    stages {
        stage('SCANOSS') {
            
            when {
                expression {
                     def payload = readJSON text: "${env.payload}"

                     return payload.pull_request !=  null && payload.pull_request.base.ref == 'main' && payload.action == 'opened'
                }
            }
            
            steps {
                script {
                    // Clean workspace before checkout
                    cleanWs()
                    
                    /***** File names *****/
                    env.SCANOSS_RESULTS_JSON_FILE = "scanoss-results.json"
                    env.SCANOSS_LICENSE_CSV_FILE = "scanoss_license_data.csv"
                    env.SCANOSS_COPYLEFT_MD_FILE = "copyleft.md"

                    /****** Create Resources folder ******/
                    env.SCANOSS_BUILD_BASE_PATH = "scanoss/${currentBuild.number}"
                    sh '''
                        mkdir -p ${SCANOSS_BUILD_BASE_PATH}/reports
                        mkdir -p ${SCANOSS_BUILD_BASE_PATH}/repository
                        mkdir -p ${SCANOSS_BUILD_BASE_PATH}/delta
                    '''
                    env.SCANOSS_REPORT_FOLDER_PATH = "${SCANOSS_BUILD_BASE_PATH}/reports"

                    /***** Resources Paths *****/
                    env.SCANOSS_LICENSE_FILE_PATH = "${env.SCANOSS_REPORT_FOLDER_PATH}/${env.SCANOSS_LICENSE_CSV_FILE}"
                    env.SCANOSS_RESULTS_FILE_PATH = "${env.SCANOSS_REPORT_FOLDER_PATH}/${SCANOSS_RESULTS_JSON_FILE}"
                    env.SCANOSS_COPYLEFT_FILE_PATH = "${env.SCANOSS_REPORT_FOLDER_PATH}/${env.SCANOSS_COPYLEFT_MD_FILE}"

                    // Checkout repository using git step
                    dir("${SCANOSS_BUILD_BASE_PATH}/repository") {
                        git branch: params.BRANCH,
                            credentialsId: params.GITHUB_TOKEN_ID,
                            url: params.GITHUB_REPO_URL
                    }
                    
                    
                    /****** Get Repository name and repo URL from payload ******/
                    def payloadJson = readJSON text: env.payload
                    env.REPOSITORY_NAME = payloadJson.pull_request.base.repo.name
                    env.REPOSITORY_URL = payloadJson.pull_request.base.repo.html_url
                    
                    // Verify checkout
                    sh """
                        echo "Repository contents after checkout:"
                        ls -la ${SCANOSS_BUILD_BASE_PATH}/repository
                    """
                    
                    // Run delta scan if enabled
                    if (params.ENABLE_DELTA_ANALYSIS) {
                        deltaScan()
                    }
                    
                    // Run SCANOSS in Docker container
                    docker.image(params.SCANOSS_CLI_DOCKER_IMAGE).inside('-u root --entrypoint=') {
                        env.SCAN_FOLDER = "${SCANOSS_BUILD_BASE_PATH}/" + (params.ENABLE_DELTA_ANALYSIS ? 'delta' : 'repository')
                        scan()
                    }
                    
                    // Process results
                    uploadArtifacts()
                }
            }
        }
    }
}

def scan() {
    withCredentials([string(credentialsId: params.SCANOSS_API_TOKEN_ID, variable: 'SCANOSS_API_TOKEN')]) {
        dir("${env.SCAN_FOLDER}") {
            script {
                sh '''
                    echo "=== Directory Information ==="
                    echo "Current working directory:"
                    pwd
                    
                    SBOM_IDENTIFY=""
                    if [ -f $SCANOSS_SBOM_IDENTIFY ]; then SBOM_IDENTIFY="--identify $SCANOSS_SBOM_IDENTIFY" ; fi

                    SBOM_IGNORE=""
                    if [ -f $SCANOSS_SBOM_IGNORE ]; then SBOM_IGNORE="--ignore $SCANOSS_SBOM_IGNORE" ; fi

                    CUSTOM_URL=""
                    if [ ! -z $SCANOSS_API_URL ]; then CUSTOM_URL="--apiurl $SCANOSS_API_URL"; else CUSTOM_URL="--apiurl https://api.scanoss.com/scan/direct" ; fi

                    CUSTOM_TOKEN=""
                    if [ ! -z $SCANOSS_API_TOKEN ]; then CUSTOM_TOKEN="--key $SCANOSS_API_TOKEN" ; fi

                    scanoss-py scan $CUSTOM_URL $CUSTOM_TOKEN $SBOM_IDENTIFY $SBOM_IGNORE --output $WORKSPACE/$SCANOSS_REPORT_FOLDER_PATH/$SCANOSS_RESULTS_JSON_FILE .
                '''
            }
        }
    }
}

def uploadArtifacts() {
    def scanossResultsPath = "${env.SCANOSS_RESULTS_FILE_PATH}"
    archiveArtifacts artifacts: scanossResultsPath, onlyIfSuccessful: true
}

def deltaScan() {
    echo 'Delta Scan Analysis enabled'

    def payloadJson = readJSON text: env.payload
    def commits = payloadJson.commits
    def destinationFolder = "${env.SCANOSS_BUILD_BASE_PATH}/delta"
    def uniqueFileNames = new HashSet()

    commits.each { commit ->
        commit.modified.each { fileName ->
            uniqueFileNames.add(fileName.trim())
        }
        commit.added.each { fileName ->
            uniqueFileNames.add(fileName.trim())
        }
    }

    dir("${env.SCANOSS_BUILD_BASE_PATH}/repository") {
        uniqueFileNames.each { file ->
            def sourcePath = "${file}"
            def destinationPath = "${destinationFolder}"
            sh "cp --parents ${sourcePath} ${destinationPath}"
        }
    }
}
