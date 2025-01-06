pipeline {
    agent any
    
    parameters {

        string(name: 'SCANOSS_API_TOKEN_ID', defaultValue:"scanoss-token", description: 'The reference ID for the SCANOSS API TOKEN credential')
        string(name: 'SCANOSS_CLI_DOCKER_IMAGE', defaultValue:"ghcr.io/scanoss/scanoss-py:v1.19.0", description: 'SCANOSS CLI Docker Image')
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
        

        string(name: 'JIRA_TOKEN_ID', defaultValue:"jira-token" , description: 'Jira token id')
        string(name: 'JIRA_URL', defaultValue:"https://scanoss.atlassian.net/" , description: 'Jira URL')
        string(name: 'JIRA_PROJECT_KEY', defaultValue:"TESTPROJ" , description: 'Jira Project Key')
        booleanParam(name: 'CREATE_JIRA_ISSUE', defaultValue: false, description: 'Enable Jira reporting')
        booleanParam(name: 'ABORT_ON_POLICY_FAILURE', defaultValue: false, description: 'Abort Pipeline on pipeline Failure')
    }

    environment {
        SCANOSS_COPYLEFT_POLICY_FILE_NAME = "scanoss-copyleft-policy.md"  // Default path, can be overridden
        SCANOSS_UNDECLARED_COMPONENT_POLICY_FILE_NAME = "scanoss-undeclared-components-policy.md"
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
                   env.POLICY_RESULTS = '0'
                   scan() 
                   copyleftPolicyCheck()
                   undeclaredComponentsPolicyCheck()
                   if (env.POLICY_RESULTS == '1') {
                       currentBuild.result = 'UNSTABLE'
                   }
                }   
            }
      
        }
    }
}

def undeclaredComponentsPolicyCheck() {
    script {
        def cmd = [
            'scanoss-py',
            'insp',
            'undeclared',
            '--input',
            'result.json',
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
            env.POLICY_RESULTS = '1'
            sh '''
                # Start with components file
                cat scanoss-undeclared-components.md > ${env.SCANOSS_UNDECLARED_COMPONENT_POLICY_FILE_NAME}
                
                # Append status file
                cat scanoss-undeclared-status.md >> ${env.SCANOSS_UNDECLARED_COMPONENT_POLICY_FILE_NAME}
                
                # Show final result
                echo "\n=== Final Combined Content ==="
                cat ${env.SCANOSS_UNDECLARED_COMPONENT_POLICY_FILE_NAME}
                
                chmod 644 ${env.SCANOSS_UNDECLARED_COMPONENT_POLICY_FILE_NAME}
            '''
            uploadArtifact(env.SCANOSS_UNDECLARED_COMPONENT_POLICY_FILE_NAME)
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
            'result.json',
            '--output',
            env.SCANOSS_COPYLEFT_POLICY_FILE_NAME,
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
            env.POLICY_RESULTS = '1'
            uploadArtifact(env.SCANOSS_COPYLEFT_POLICY_FILE_NAME)
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
            cmd << "--output result.json"
            
            // Execute command
            def exitCode = sh(
                script: cmd.join(' '),
                returnStatus: true
            )
            
            if (exitCode != 0) {
                echo "Warning: Scan failed with exit code ${exitCode}"
            }
            
            uploadArtifact('result.json')
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


def createCopyleftJiraIssue(){
    if (!fileExists(env.SCANOSS_COPYLEFT_POLICY_FILE_NAME)) {
            error "File '${env.SCANOSS_COPYLEFT_POLICY_FILE_NAME}' does not exist"
        }
        
        def mdContent = readFile(env.SCANOSS_COPYLEFT_POLICY_FILE_NAME).trim()
        if (mdContent.isEmpty()) {
            error "File '${env.SCANOSS_COPYLEFT_POLICY_FILE_NAME}' is empty"
        }
        
        echo "Content found in file. Creating JIRA issue..."
        
        // Create JIRA issue
        createJiraIssue(
            projectKey: params.JIRA_PROJECT_KEY,
            summary: "Issue from Pipeline: ${env.JOB_NAME}",
            description: mdContent,
            issueType: 'Bug'
        )
    }
}

def createJiraIssue(Map config) {
    withCredentials([usernamePassword(
        credentialsId: params.JIRA_CREDS_ID,
        usernameVariable: 'JIRA_USER',
        passwordVariable: 'JIRA_TOKEN'
    )]) {
        def payload = [
            fields: [
                project: [key: config.projectKey],
                summary: config.summary,
                description: config.description,
                issuetype: [name: config.issueType]
            ]
        ]
        
        try {
            def response = httpRequest(
                url: "${params.JIRA_URL}/rest/api/2/issue/",
                httpMode: 'POST',
                contentType: 'APPLICATION_JSON',
                customHeaders: [[
                    name: 'Authorization',
                    value: "Basic ${("${JIRA_USER}:${JIRA_TOKEN}").bytes.encodeBase64().toString()}"
                ]],
                requestBody: groovy.json.JsonOutput.toJson(payload)
            )
            
            echo "JIRA Issue created: ${response.content}"
            return response.content
        } catch (Exception e) {
            error "Failed to create JIRA issue: ${e.message}"
        }
    }
}
