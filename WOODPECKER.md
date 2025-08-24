# Woodpecker CI Pipeline Documentation

This document explains the Woodpecker CI pipeline configuration that replaces the GitHub Actions workflow for building and publishing the game server Docker images.

## Available Pipeline Configurations

Three different Woodpecker CI configurations are provided to suit different needs:

1. **`.woodpecker.yml`** - **Recommended**: Full-featured pipeline with explicit steps for maximum control
2. **`.woodpecker-matrix.yml`** - Matrix-based approach similar to GitHub Actions structure  
3. **`.woodpecker-simple.yml`** - Simplified single-step multi-arch build for basic use cases

Choose the one that best fits your Woodpecker CI setup and requirements.

### Configuration Comparison

| Feature | `.woodpecker.yml` | `.woodpecker-matrix.yml` | `.woodpecker-simple.yml` |
|---------|------------------|-------------------------|-------------------------|
| **Architecture Handling** | Explicit steps per arch | Matrix strategy | Single multi-arch step |
| **Manifest Creation** | Explicit steps | Separate pipeline | Automatic |
| **Complexity** | High | Medium | Low |
| **Control Level** | Maximum | High | Basic |
| **GitHub Actions Similarity** | Different approach | Very similar | Simplified |
| **Recommended For** | Production | Migration from GHA | Testing/Simple setups |

## Overview

The Woodpecker CI pipeline (`.woodpecker.yml`) provides the same functionality as the original GitHub Actions workflow (`.github/workflows/build-game-server.yaml`):

- **Pull Request Testing**: Builds Docker images for both AMD64 and ARM64 architectures without pushing to registry
- **Branch Builds**: Builds and pushes multi-architecture Docker images to GitHub Container Registry
- **Multi-Architecture Support**: Creates unified manifests combining AMD64 and ARM64 images
- **Caching**: Utilizes Docker layer caching for faster builds

## Key Differences from GitHub Actions

### 1. Configuration Structure
- **GitHub Actions**: Uses `jobs` with `strategy.matrix` for multi-architecture builds
- **Woodpecker**: Uses `steps` with individual step definitions for each architecture

### 2. Conditional Execution
- **GitHub Actions**: Uses `if` conditions
- **Woodpecker**: Uses `when` conditions

### 3. Secrets Management
- **GitHub Actions**: Uses `${{ secrets.GITHUB_TOKEN }}`
- **Woodpecker**: Uses `from_secret` syntax

### 4. Variable Syntax
- **GitHub Actions**: Uses `${{ github.ref_name }}` and `${{ github.sha }}`
- **Woodpecker**: Uses `${CI_COMMIT_BRANCH}` and `${CI_COMMIT_SHA}`

## Pipeline Structure

### Pull Request Flow
1. `test-build-amd64` - Tests AMD64 build without pushing
2. `test-build-arm64` - Tests ARM64 build without pushing

### Push Flow
1. `build-amd64` - Builds and pushes AMD64 image with architecture-specific tags
2. `build-arm64` - Builds and pushes ARM64 image with architecture-specific tags
3. `create-manifest` - Creates multi-arch manifest for branch name tag
4. `create-manifest-sha` - Creates multi-arch manifest for commit SHA tag
5. `inspect-image` - Verifies the final multi-arch image

## Required Secrets

Configure these secrets in your Woodpecker CI instance:

- `github_username` - GitHub username for container registry authentication
- `github_token` - GitHub Personal Access Token or GitHub Token with package write permissions

## Tags Generated

For each successful build on main branches, the following tags are created:

- `{branch-name}` - Latest build for the branch (multi-arch manifest)
- `{branch-name}-{commit-sha}` - Specific commit build (multi-arch manifest)
- `{branch-name}-amd64` - AMD64 specific image
- `{branch-name}-arm64` - ARM64 specific image
- `{branch-name}-{commit-sha}-amd64` - AMD64 specific image for commit
- `{branch-name}-{commit-sha}-arm64` - ARM64 specific image for commit

## Caching

The pipeline includes Docker layer caching to speed up builds:
- Cache images are stored with tags like `:cache-amd64` and `:cache-arm64`
- `BUILDKIT_INLINE_CACHE=1` is used to embed cache metadata in images

## Triggering

The pipeline triggers on:
- **Push events** to branches: `main`, `dev`, `event-*`, `rush-*`
- **Pull request events** to any of the above branches
- **Manual triggers** via the Woodpecker UI

## Migration Notes

When migrating from GitHub Actions to Woodpecker CI:

1. **Choose Configuration**: Select one of the three provided configurations based on your needs:
   - Use `.woodpecker.yml` for production environments requiring maximum control
   - Use `.woodpecker-matrix.yml` if you prefer the GitHub Actions matrix approach
   - Use `.woodpecker-simple.yml` for simpler setups or testing

2. **Rename Configuration**: Rename your chosen file to `.woodpecker.yml` (Woodpecker's default name)

3. **Secrets**: Configure `github_username` and `github_token` secrets in Woodpecker

4. **Runners**: Ensure you have runners capable of multi-architecture builds or Docker buildx

5. **Registry Access**: Verify Woodpecker runners can access `ghcr.io`

6. **Dockerfile Path**: The pipeline uses the same Dockerfile at `.github/workflows/game-server-Dockerfile`

## Troubleshooting

### Common Issues

1. **Authentication Failures**: Check that secrets are properly configured
2. **Manifest Creation Fails**: Ensure both architecture builds completed successfully
3. **Cache Issues**: Docker buildx and manifest tools require specific setup on runners

### Debug Commands

To debug manifest issues, you can run:
```bash
docker buildx imagetools inspect ghcr.io/42core-team/game-server:branch-name
```

This will show the architecture details of the multi-arch image.