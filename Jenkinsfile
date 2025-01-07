
pipeline {
    agent any
    
    parameters {

        string(name: 'SCANOSS_API_TOKEN_ID', defaultValue:"scanoss-token", description: 'The reference ID for the SCANOSS API TOKEN credential')
        string(name: 'SCANOSS_CLI_DOCKER_IMAGE', defaultValue:"ghcr.io/scanoss/scanoss-py:v1.19.1", description: 'SCANOSS CLI Docker Image')
        string(name: 'SCANOSS_API_URL', defaultValue:"https://api.osskb.org/scan/direct", description: 'SCANOSS API URL (optional - default: https://api.osskb.org/scan/direct)')


        booleanParam(name: 'SKIP_SNIPPET', defaultValue: false, description: 'Skip the generation of snippets.')
        booleanParam(name: 'SCANOSS_SETTINGS', defaultValue: true, description: 'Settings file to use for scanning.')
        string(name: 'SETTINGS_FILE_PATH', defaultValue: 'scanoss.json', description: 'SCANOSS settings file path.')

        // Dependencies
        booleanParam(name: 'DEPENDENCY_ENABLED', defaultValue: false, description: 'Scan dependencies (optional - default false).')
        string(name: 'DEPENDENCY_SCOPE', defaultValue: '', description: 'Gets development or production dependencies (scopes - prod|dev)')
        string(name: 'DEPENDENCY_SCOPE_INCLUDE', defaultValue: '', description: 'Custom list of dependency scopes to be included. Provide scopes as a comma-separated list.')
        string(name: 'DEPENDENCY_SCOPE_EXCLUDE', defaultValue: '', description: 'Custom list of dependency scopes to be excluded. Provide scopes as a comma-separated list.')

        // Copyleft licenses
        string(name: 'LICENSES_COPYLEFT_INCLUDE', defaultValue: 'MIT', description: 'List of Copyleft licenses to append to the default list. Provide licenses as a comma-separated list.')
        string(name: 'LICENSES_COPYLEFT_EXCLUDE', defaultValue: '', description: 'List of Copyleft licenses to remove from default list. Provide licenses as a comma-separated list.')
        string(name: 'LICENSES_COPYLEFT_EXPLICIT', defaultValue: '', description: 'Explicit list of Copyleft licenses to consider. Provide licenses as a comma-separated list.')
        

        string(name: 'JIRA_CREDENTIALS', defaultValue:"jira-credentials" , description: 'Jira credentials')
        string(name: 'JIRA_URL', defaultValue:"https://scanoss.atlassian.net/" , description: 'Jira URL')
        string(name: 'JIRA_PROJECT_KEY', defaultValue:"TESTPROJ" , description: 'Jira Project Key')
        booleanParam(name: 'CREATE_JIRA_ISSUE', defaultValue: true, description: 'Enable Jira reporting')
        booleanParam(name: 'ABORT_ON_POLICY_FAILURE', defaultValue: false, description: 'Abort Pipeline on pipeline Failure')
    }

    environment {
        
        // Artifact file names
        SCANOSS_COPYLEFT_REPORT_MD = "scanoss-copyleft-report.md"
        SCANOSS_UNDECLARED_REPORT_MD = "scanoss-undeclared-report.md"
        SCANOSS_RESULTS_OUTPUT_FILE_NAME = "results.json"
        
        
        // Policies status
        COPYLEFT_POLICY_STATUS = '0'
        UNDECLARED_POLICY_STATUS = '0'
        
        // Markdwon Jira report file names
        SCANOSS_COPYLEFT_JIRA_REPORT_MD = "scanoss-copyleft-jira_report.md"
        SCANOSS_UNDECLARED_JIRA_REPORT_MD = "scanoss-undeclared-components-jira-report.md"
    }

    stages {
        stage('SCANOSS') {
            agent {
                docker {
                    image params.SCANOSS_CLI_DOCKER_IMAGE
                    args '-u root --entrypoint='
                    // Run the container on the node specified at the
                    // top-level of the Pipeline, in the same workspace,
                    // rather than on a new node entirely:
                    reuseNode true
                }
            }
            steps {
               script {
                   scan() 
                   copyleftPolicyCheck()
                   undeclaredComponentsPolicyCheck()


                    // Create Jira issues if enabled
                    if (params.CREATE_JIRA_ISSUE) {
                        if (env.COPYLEFT_POLICY_STATUS == '1') {
                            createJiraTicket("Copyleft licenses found", env.SCANOSS_COPYLEFT_JIRA_REPORT_MD)
                        }
                        
                        if (env.UNDECLARED_POLICY_STATUS == '1') {
                            createJiraTicket("Undeclared components found", env.SCANOSS_UNDECLARED_JIRA_REPORT_MD)
                        }
                    }
                   
                   
                   // Set build status based on policies
                   if (env.COPYLEFT_POLICY_STATUS == '1' || env.UNDECLARED_POLICY_STATUS == '1') {
                       currentBuild.result = 'UNSTABLE'
                   }
                }   
            }
      
        }
    }
}

def createJiraMarkdownUndeclaredComponentReport(){
        script {
        def cmd = [
            'scanoss-py',
            'insp',
            'undeclared',
            '--input',
            env.SCANOSS_RESULTS_OUTPUT_FILE_NAME,
            '--output',
            'scanoss-undeclared-components-jira.md',
            '--status',
            'scanoss-undeclared-status-jira.md',
            '-f',
            'jira_md']

        def exitCode = sh(
            script: cmd.join(' '),
            returnStatus: true
        )   
        
        if (exitCode == 0) {
            sh '''
                # Start with components file
                cat scanoss-undeclared-status-jira.md scanoss-undeclared-components-jira.md | tr '\n' '\\n' > ${env.SCANOSS_UNDECLARED_JIRA_REPORT_MD}
                
                # Show final result
                echo "\n=== Final Combined Content ==="
                cat ${env.SCANOSS_UNDECLARED_JIRA_REPORT_MD}
                
                chmod 644 ${env.SCANOSS_UNDECLARED_JIRA_REPORT_MD}
            '''
        }
    }
}


def createJiraMarkdownCopyleftReport(){
        script {
        def cmd = [
            'scanoss-py',
            'insp',
            'copyleft',
            '--input',
            env.SCANOSS_RESULTS_OUTPUT_FILE_NAME,
            '--output',
            env.SCANOSS_COPYLEFT_JIRA_REPORT_MD,
            '-f',
            'jira_md']

        // Copyleft licenses
        cmd.addAll(buildCopyleftArgs())
        
        def exitCode = sh(
            script: cmd.join(' '),
            returnStatus: true
        )        
            
    }
}


def undeclaredComponentsPolicyCheck() {
    script {
        def cmd = [
            'scanoss-py',
            'insp',
            'undeclared',
            '--input',
            env.SCANOSS_RESULTS_OUTPUT_FILE_NAME,
            '--output',
            'scanoss-undeclared-components.md',
            '--status',
            'scanoss-undeclared-status.md',
            '-f',
            'md']

        def exitCode = sh(
            script: cmd.join(' '),
            returnStatus: true
        )
        
        if (exitCode == 1) {
            echo "No Undeclared components were found"
        } else {
            env.UNDECLARED_POLICY_STATUS = '1'
            sh '''
                # Start with components file
                cat scanoss-undeclared-components.md > ${env.SCANOSS_UNDECLARED_REPORT_MD}
                
                # Append status file
                cat scanoss-undeclared-status.md >> ${env.SCANOSS_UNDECLARED_REPORT_MD}
                
                # Show final result
                echo "\n=== Final Combined Content ==="
                cat ${env.SCANOSS_UNDECLARED_REPORT_MD}
                
                chmod 644 ${env.SCANOSS_UNDECLARED_REPORT_MD}
            '''
            uploadArtifact(env.SCANOSS_UNDECLARED_REPORT_MD)
        }
    }
}

def copyleftPolicyCheck() {
    script {
        def cmd = [
            'scanoss-py',
            'insp',
            'copyleft',
            '--input',
            env.SCANOSS_RESULTS_OUTPUT_FILE_NAME,
            '--output',
            env.SCANOSS_COPYLEFT_REPORT_MD,
            '-f',
            'md']

        // Copyleft licenses
        cmd.addAll(buildCopyleftArgs())
        
        def exitCode = sh(
            script: cmd.join(' '),
            returnStatus: true
        )        
            
        if (exitCode == 1) {
            echo "No copyleft licenses were found"
        } else {
            env.COPYLEFT_POLICY_STATUS = '1'
            uploadArtifact(env.SCANOSS_COPYLEFT_REPORT_MD)
        }
    }
}

def scan() {
    withCredentials([string(credentialsId: params.SCANOSS_API_TOKEN_ID, variable: 'SCANOSS_API_TOKEN')]) {
        script {
            def cmd = []
            cmd << "scanoss-py scan"
            
            // Add target directory
            cmd << "."
            
            // Add API URL
            cmd << "--apiurl ${SCANOSS_API_URL}"
            
            // Add API token if available
            if (env.SCANOSS_API_TOKEN) {
                cmd << "--key ${SCANOSS_API_TOKEN}"
            }
            
            // Skip Snippet
            if (env.SKIP_SNIPPET == 'true') {
               cmd << "-S"
            }

            // Settings
            if (env.SCANOSS_SETTINGS == 'true') {
               cmd << "--settings ${env.SETTINGS_FILE_PATH}"
            } else {
               cmd << "-stf"
            }

           // Dependency Scope
            if (env.DEPENDENCY_ENABLED == 'true') {
               cmd << buildDependencyScopeArgs()
            }
           
            // Add output file
            cmd << "--output ${env.SCANOSS_RESULTS_OUTPUT_FILE_NAME}"
            
            // Execute command
            def exitCode = sh(
                script: cmd.join(' '),
                returnStatus: true
            )
            
            if (exitCode != 0) {
                echo "Warning: Scan failed with exit code ${exitCode}"
            }
            
            uploadArtifact(env.SCANOSS_RESULTS_OUTPUT_FILE_NAME)
        }
    }
}

def uploadArtifact(artifactPath) {
    archiveArtifacts artifacts: artifactPath, onlyIfSuccessful: true
}

def List<String> buildDependencyScopeArgs() {
    def dependencyScopeInclude = params.DEPENDENCY_SCOPE_INCLUDE
    def dependencyScopeExclude = params.DEPENDENCY_SCOPE_EXCLUDE
    def dependencyScope = params.DEPENDENCY_SCOPE

    // Count the number of non-empty values
    def setScopes = [dependencyScopeInclude, dependencyScopeExclude, dependencyScope].findAll {
        it != '' && it != null
    }

    if (setScopes.size() > 1) {
        core.error('Only one dependency scope filter can be set')
    }

    if (dependencyScopeExclude && dependencyScopeExclude != '') {
        return ['--dep-scope-exc', dependencyScopeExclude]
    }
    if (dependencyScopeInclude && dependencyScopeInclude != '') {
        return ['--dep-scope-inc',dependencyScopeInclude]
    }
    if (dependencyScope && dependencyScope == 'prod') {
        return ['--dep-scope', 'prod']
    }
    if (dependencyScope && dependencyScope == 'dev') {
        return ['--dep-scope', 'dev']
    }

    return ''
}

def List<String> buildCopyleftArgs() {
    if (params.LICENSES_COPYLEFT_EXPLICIT != '') {
        println "Explicit copyleft licenses: ${params.LICENSES_COPYLEFT_EXPLICIT}"
        return ['--explicit', params.LICENSES_COPYLEFT_EXPLICIT]
    }
    if (params.LICENSES_COPYLEFT_INCLUDE != '') {
        println "Included copyleft licenses: ${params.LICENSES_COPYLEFT_INCLUDE}"
        return ['--include', params.LICENSES_COPYLEFT_INCLUDE]
    }
    if (params.LICENSES_COPYLEFT_EXCLUDE != '') {
        println "Excluded copyleft licenses: ${params.LICENSES_COPYLEFT_EXCLUDE}"
        return ['--exclude', params.LICENSES_COPYLEFT_EXCLUDE]
    }
    return []
}

def createJiraTicket(String title, String filePath) {
    def jiraEndpoint = "${params.JIRA_URL}/rest/api/2/issue/"
    
    withCredentials([usernamePassword(credentialsId: params.JIRA_CREDENTIALS,
                    usernameVariable: 'JIRA_USER',
                    passwordVariable: 'JIRA_TOKEN')]) {
        try {
            // Read file content
            def fileContent = ""
            if (fileExists(filePath)) {
                fileContent = readFile(file: filePath)
            } else {
                error "File ${filePath} not found"
            }

         

            // Prepare JIRA ticket payload
            def payload = [
                fields: [
                    project: [key: params.JIRA_PROJECT_KEY],
                    summary: title,
                    description: fileContent,
                    issuetype: [name: 'Bug']
                ]
            ]

            
            // Convert payload to JSON string
            def jsonString = groovy.json.JsonOutput.toJson(payload)
            
            // Create curl command
            def curlCommand = """
                curl -s -u '${JIRA_USER}:${JIRA_TOKEN}' \
                    -X POST \
                    -H 'Content-Type: application/json' \
                    -d '${jsonString}' \
                    '${jiraEndpoint}'
            """
            
            // Execute curl command
            def response = sh(script: curlCommand, returnStdout: true).trim()
            
            echo "JIRA ticket created successfully"
            return response

        } catch (Exception e) {
            echo "Failed to create JIRA ticket: ${e.message}"
            error "JIRA ticket creation failed"
        }
    }
}
