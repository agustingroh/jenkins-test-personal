pipeline {
    agent any
    
    parameters {
        string(name: 'BRANCH', defaultValue: 'main', description: 'Branch to build')
        string(name: 'GITHUB_REPO_URL', defaultValue: 'https://github.com/agustingroh/jenkins-test-personal/', description: 'GitHub repository URL')
        string(name: 'SCANOSS_API_TOKEN_ID', defaultValue:"scanoss-token", description: 'The reference ID for the SCANOSS API TOKEN credential')
        string(name: 'SCANOSS_SBOM_IDENTIFY', defaultValue:"sbom.json", description: 'SCANOSS SBOM Identify filename')
        string(name: 'SCANOSS_SBOM_IGNORE', defaultValue:"sbom-ignore.json", description: 'SCANOSS SBOM Ignore filename')
        string(name: 'SCANOSS_CLI_DOCKER_IMAGE', defaultValue:"ghcr.io/scanoss/scanoss-py:v1.9.0", description: 'SCANOSS CLI Docker Image')
        booleanParam(name: 'ENABLE_DELTA_ANALYSIS', defaultValue: true, description: 'Analyze those files what have changed or new ones')
        string(name: 'JIRA_TOKEN_ID', defaultValue:"jira-token" , description: 'Jira token id')
        string(name: 'JIRA_URL', defaultValue:"https://scanoss.atlassian.net/" , description: 'Jira URL')
        string(name: 'JIRA_PROJECT_KEY', defaultValue:"TESTPROJ" , description: 'Jira Project Key')
        booleanParam(name: 'CREATE_JIRA_ISSUE', defaultValue: false, description: 'Enable Jira reporting')
        booleanParam(name: 'ABORT_ON_POLICY_FAILURE', defaultValue: false, description: 'Abort Pipeline on pipeline Failure')
    }

    stages {
        stage('SCANOSS') {
            
            when {
                expression {
                     def payload = readJSON text: "${env.payload}"

                     return (payload.pull_request !=  null && payload.pull_request.base.ref == 'main' && payload.action == 'opened') || payload.ref != null
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
                    env.SCANOSS_DELTA_DIR = "${env.SCANOSS_BUILD_BASE_PATH}/delta"
                    env.SCANOSS_REPO_DIR = "${env.SCANOSS_BUILD_BASE_PATH}/repository"

                    /****** Create Resources folder ******/
                    env.SCANOSS_BUILD_BASE_PATH = "${env.WORKSPACE}/scanoss/${currentBuild.number}"

                    echo "=== Path Information ==="
                    echo "Workspace: ${env.WORKSPACE}"
                    echo "Build Number: ${currentBuild.number}"
                    echo "SCANOSS Build Base Path: ${env.SCANOSS_BUILD_BASE_PATH}"

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
                    if (payloadJson.pull_request != null) {
                        env.REPOSITORY_NAME = payloadJson.pull_request.base.repo.name
                        env.REPOSITORY_URL = payloadJson.pull_request.base.repo.html_url
                    }

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
                        env.SCAN_FOLDER = (params.ENABLE_DELTA_ANALYSIS ? ${env.SCANOSS_DELTA_DIR} : ${env.SCANOSS_REPO_DIR})
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
 echo "Starting Delta Analysis..."
    try {
        // First, check if payload exists
        if (!env.payload) {
            echo "Warning: No payload found. This might not be a PR trigger."
            return
        }

        def payloadJson = readJSON text: env.payload
        echo "Payload parsed successfully"
        
        // Check if commits exist in payload
        if (!payloadJson.commits) {
            echo "Warning: No commits found in payload"
            return
        }
        
        def commits = payloadJson.commits
        def destinationFolder = "${env.SCANOSS_BUILD_BASE_PATH}/delta"
        def uniqueFileNames = new HashSet()
        
        echo "Number of commits found: ${commits.size()}"
        
        commits.each { commit ->
            echo "\n=== Processing Commit ==="
            if (commit.id) {
                echo "Commit ID: ${commit.id}"
            }
            
            // Safely handle modified files
            if (commit.modified) {
                echo "Processing modified files..."
                commit.modified.each { fileName ->
                    if (fileName) {
                        echo "Adding modified file: ${fileName}"
                        uniqueFileNames.add(fileName.trim())
                    }
                }
            } else {
                echo "No modified files in this commit"
            }
            
            // Safely handle added files
            if (commit.added) {
                echo "Processing added files..."
                commit.added.each { fileName ->
                    if (fileName) {
                        echo "Adding new file: ${fileName}"
                        uniqueFileNames.add(fileName.trim())
                    }
                }
            } else {
                echo "No added files in this commit"
            }
        }
        
        if (uniqueFileNames.isEmpty()) {
            echo "No files to process. Skipping file copy."
            return
        }
        
        echo "\nTotal files to process: ${uniqueFileNames.size()}"
        
        dir("${env.SCANOSS_REPO_DIR}") {
            uniqueFileNames.each { file ->
                sh """
                          # Check if directories exist
                          echo "Checking directories..."
                          echo "Current directory: \$(pwd)"
                          echo "Delta directory: ${destinationFolder}"

                          if [ -d "${env.SCANOSS_DELTA_DIR}" ]; then
                              echo "Delta directory exists"
                              if [ -f "${file}" ]; then
                                  echo "Source file exists: ${file}"
                                  cp --parents '${file}' '${env.SCANOSS_DELTA_DIR}'
                              else
                                  echo "Warning: Source file not found: ${file}"
                              fi
                          else
                              echo "Error: Delta directory does not exist: ${destinationFolder}"
                              exit 1
                          fi
                      """
            }
        }
        
        echo "Delta Analysis completed successfully"
        
    } catch (Exception e) {
        echo "Error in Delta Analysis: ${e.getMessage()}"
        echo "Error details: ${e}"
        throw e
    }
}
