pipeline {
    agent any

    parameters {
        string(name: 'SCANOSS_API_TOKEN_ID', defaultValue:"scanoss-token", description: 'The reference ID for the SCANOSS API TOKEN credential')
        string(name: 'SCANOSS_CLI_DOCKER_IMAGE', defaultValue:"ghcr.io/scanoss/scanoss-py:v1.19.0", description: 'SCANOSS CLI Docker Image')
        string(name: 'SCANOSS_API_URL', defaultValue:"https://api.osskb.org/scan/direct", description: 'SCANOSS API URL (optional - default: https://api.osskb.org/scan/direct)')

        booleanParam(name: 'ENABLE_DELTA_ANALYSIS', defaultValue: false, description: 'Analyze those files what have changed or new ones')
        booleanParam(name: 'SKIP_SNIPPET', defaultValue: false, description: 'Skip the generation of snippets.')
        booleanParam(name: 'SCANOSS_SETTINGS', defaultValue: false, description: 'Settings file to use for scanning.')
        string(name: 'SETTINGS_FILE_PATH', defaultValue: 'scanoss.json', description: 'SCANOSS settings file path.')

        booleanParam(name: 'DEPENDENCY_ENABLED', defaultValue: false, description: 'Scan dependencies (optional - default false).')
        string(name: 'DEPENDENCY_SCOPE', defaultValue: '', description: 'Gets development or production dependencies (scopes - prod|dev)')
        string(name: 'DEPENDENCY_SCOPE_INCLUDE', defaultValue: '', description: 'Custom list of dependency scopes to be included. Provide scopes as a comma-separated list.')
        string(name: 'DEPENDENCY_SCOPE_EXCLUDE', defaultValue: '', description: 'Custom list of dependency scopes to be excluded. Provide scopes as a comma-separated list.')


        string(name: 'JIRA_TOKEN_ID', defaultValue:"jira-token" , description: 'Jira token id')
        string(name: 'JIRA_URL', defaultValue:"https://scanoss.atlassian.net/" , description: 'Jira URL')
        string(name: 'JIRA_PROJECT_KEY', defaultValue:"TESTPROJ" , description: 'Jira Project Key')
        booleanParam(name: 'CREATE_JIRA_ISSUE', defaultValue: false, description: 'Enable Jira reporting')
        booleanParam(name: 'ABORT_ON_POLICY_FAILURE', defaultValue: false, description: 'Abort Pipeline on pipeline Failure')
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
        def cmd = []
        cmd << "scanoss-py insp undeclared"
        cmd << "--input results.json"
        cmd << "--output scanoss-undeclared-components.md"
        cmd << "--status scanoss-undeclared-status.md"
        cmd << "-f md"

        def exitCode = sh(
            script: cmd.join(' '),
            returnStatus: true
        )

        if (exitCode != 0) {
            echo "Warning: Copyleft inspection failed with exit code ${exitCode}"
            currentBuild.result = 'UNSTABLE'
            env.POLICY_RESULTS = '1'
        } else {
            sh '''
                # Start with components file
                cat scanoss-undeclared-components.md > scanoss-undeclared-components-policy.md

                # Append status file
                cat scanoss-undeclared-status.md >> scanoss-undeclared-components-policy.md

                # Show final result
                echo "\n=== Final Combined Content ==="
                cat scanoss-undeclared-components-policy.md

                chmod 644 scanoss-undeclared-components-policy.md
            '''
            uploadArtifact('scanoss-undeclared-components-policy.md')
        }
    }
}

def copyleftPolicyCheck() {
    script {
        def cmd = []
        cmd << "scanoss-py insp copyleft"
        cmd << "--input results.json"
        cmd << "--output scanoss-copyleft-policy.md"
        cmd << "-f md"

        def exitCode = sh(
            script: cmd.join(' '),
            returnStatus: true
        )

        if (exitCode != 0) {
            echo "Warning: Copyleft inspection failed with exit code ${exitCode}"
            env.POLICY_RESULTS = '1'
        } else {
            uploadArtifact('scanoss-copyleft-policy.md')
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
               cmd << dependencyScopeArgs()
           }

            // Add output file
            cmd << "--output results.json"

            // Execute command
            def exitCode = sh(
                script: cmd.join(' '),
                returnStatus: true
            )

            if (exitCode != 0) {
                echo "Warning: Scan failed with exit code ${exitCode}"
            }

            uploadArtifact('results.json')
        }
    }
}

def uploadArtifact(artifactPath) {
    archiveArtifacts artifacts: artifactPath, onlyIfSuccessful: true
}

def dependencyScopeArgs() {
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
        return '--dep-scope-exc ' + dependencyScopeExclude
    }
    if (dependencyScopeInclude && dependencyScopeInclude != '') {
        return '--dep-scope-inc ' + dependencyScopeInclude
    }
    if (dependencyScope && dependencyScope == 'prod') {
        return '--dep-scope prod'
    }
    if (dependencyScope && dependencyScope == 'dev') {
        return '--dep-scope dev'
    }

    return ''
}
