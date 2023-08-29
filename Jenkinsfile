pipeline {
  agent {
     node { 
        label 'UbuntuBionics32'
        } 
  }
  stages {
    stage('Checkout Scm') {
      steps {
        git 'eolJenkins:NCAR/aircraft_ipm.git'
      }
    }
    stage('Build') {
      steps {
        sh 'scons'
        sh 'scons tests'
        sh 'tests/g_test'
      }
    }
  }
  post {
    failure {
      mail(subject: 'iPM Jenkinsfile build failed', body: 'See build console output within jenkins for details', to: 'janine@ucar.edu')
    }

  }
  options {
    buildDiscarder(logRotator(numToKeepStr: '6'))
  }
}
