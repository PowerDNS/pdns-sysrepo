# Updating the code
 First, update the `authoritative-api-openapi.yaml`

 ```bash
 openapi-generator generate -i authoritative-api-openapi.yaml -g cpp-restsdk -o include
 ```