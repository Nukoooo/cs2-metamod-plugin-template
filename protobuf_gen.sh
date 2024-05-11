current_dir="${PWD}"

cd hl2sdk
protobuf_exec="${PWD}/hl2sdk/devtools/bin/linux/protoc"

${PWD}/devtools/bin/linux/protoc --proto_path=thirdparty/protobuf-3.21.8/src --proto_path=common --cpp_out=common common/network_connection.proto
