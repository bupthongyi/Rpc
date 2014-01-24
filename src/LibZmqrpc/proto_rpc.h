#include "zmq_rpc.h"
#include "proto_serializer.h"
namespace zmq_rpc {

// ------------------------------------------------------------
// T is a protobuf object. wrapper it to be a Request.
template <class T>
class RPCRequest : public Request, public T {
public:
	virtual ~RPCRequest() {
	}
	virtual bool serialize(WriteableByteArray* bytes) const;
	virtual bool deserialize(ReadableByteArray* bytes);
}; // class RPCRequest

template <class T>
bool RPCRequest<T>::serialize(WriteableByteArray* bytes) const {
	return SerializeToWriteBuffer(this, bytes);
}

template <class T>
bool RPCRequest<T>::deserialize(ReadableByteArray* bytes) {
	return DeserizlizeFromReadArray(this, bytes);
}

// ------------------------------------------------------------
// T is a protobuf object. wrapper it to be a Response.
template <class T>
class RPCResponse : public T, public Response {
public:
	virtual ~RPCResponse() {
	}
	virtual bool serialize(WriteableByteArray* bytes) const;
	virtual bool deserialize(ReadableByteArray* bytes);
}; // class RPCResponse

template <class T>
bool RPCResponse<T>::serialize(WriteableByteArray* bytes) const {
	return SerializeToWriteBuffer(this, bytes);
}

template <class T>
bool RPCResponse<T>::deserialize(ReadableByteArray* bytes) {
	return DeserizlizeFromReadArray(this, bytes);
}

// ------------------------------------------------------------
// REQUEST and RESPONSE are protobuf objects.
template <class REQUEST, class RESPONSE>
class RPCService : public Service {
public:
	virtual zmq_rpc::Request* allocateRequest();
	virtual zmq_rpc::Response* allocateResponse();
	virtual void run(const zmq_rpc::Request* request, zmq_rpc::Response* response);
	virtual std::string handleRequest(const REQUEST* req, RESPONSE* res) = 0;
}; // class RPCService.

template <class REQUEST, class RESPONSE>
zmq_rpc::Request* RPCService<REQUEST, RESPONSE>::allocateRequest() {
	return new RPCRequest<REQUEST>();
}

template <class REQUEST, class RESPONSE>
zmq_rpc::Response* RPCService<REQUEST, RESPONSE>::allocateResponse() {
	return new RPCResponse<RESPONSE>();
}

template <class REQUEST, class RESPONSE>
void RPCService<REQUEST, RESPONSE>::run(const Request* request, Response* response) {
	const RPCRequest<REQUEST>* req = dynamic_cast<const RPCRequest<REQUEST>*>(request);
	RPCResponse<RESPONSE>* res = dynamic_cast<RPCResponse<RESPONSE>*>(response);
	std::string return_value = handleRequest(req, res);
	res->set_return_value(return_value.c_str());
}

} // namespace proto_rpc