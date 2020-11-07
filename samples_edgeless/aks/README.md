# Deploy an Edgeless RT App on Azure Kubernetes Service (AKS)

This sample shows how to deploy an ERT App on AKS.

## 0. Build the Docker image
We provide a sample image that is used in the following steps. If you want to build it yourself (or your own app), you need to generate a signing key and then build and push the image:
```
openssl genrsa -out private.pem -3 3072
openssl rsa -in private.pem -pubout -out public.pem
DOCKER_BUILDKIT=1 docker build --secret id=signingkey,src=private.pem --tag ghcr.io/OWNER/ert-sample .
docker push ghcr.io/OWNER/ert-sample
```

## 1. Deploy on AKS

### Prerequisites: Setting up an [AKS cluster](https://docs.microsoft.com/en-us/azure/confidential-computing/confidential-nodes-aks-get-started)
* Note: see available [VM sizes](https://docs.microsoft.com/en-us/azure/virtual-machines/dcv2-series) and supported [regions](https://azure.microsoft.com/en-us/global-infrastructure/services/?products=virtual-machines&regions=all#select-product) for confidential nodes

### Push configuration to cluster
* switch to AKS context:
  ```
  kubectl config use-context myAKSCluster
  ```
* apply [deployment](aks.yml) to cluster:
  ```
  kubectl apply -f aks.yml
  ```

### Verify that the pod is running
```
$ kubectl get pods

NAME             READY   STATUS    RESTARTS   AGE
ert-sample-pod   1/1     Running   0          20s

$ kubectl logs ert-sample-pod

[erthost] loading enclave ...
[erthost] entering enclave ...
[deventry] running in development mode
[deventry] invoking main
listening ...
```

## 2. Use the service
* obtain IP from service:
  ```
  $ kubectl get service

  NAME             TYPE           CLUSTER-IP    EXTERNAL-IP    PORT(S)          AGE
  ert-sample-svc   LoadBalancer   10.0.216.92   <external-ip>  8080:32177/TCP   20s
  ```
* use the client:
  ```
  $ cd ../go_ra
  $ go run client.go -s `oesign signerid -k ../aks/public.pem` -a <external-ip>:8080

  Sent secret over attested TLS channel.
  ```
## Managing the cluster
* start cluster: ```az aks start --name myAKSCluster --resource-group myResourceGroup```
* stop cluster: ```az aks stop --name myAKSCluster --resource-group myResourceGroup```

## References
* [az aks documentation](https://docs.microsoft.com/en-us/cli/azure/aks?view=azure-cli-latest)
* [kubectl cheat sheet](https://kubernetes.io/de/docs/reference/kubectl/cheatsheet/)
