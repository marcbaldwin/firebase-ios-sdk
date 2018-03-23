const assert = require('assert');
const functions = require('firebase-functions');

exports.dataTest = functions.https.onRequest((request, response) => {
  assert.deepEqual(request.body, {
    data: {
      bool: true,
      int: 2,
      long: {
        value: '3',
        '@type': 'type.googleapis.com/google.protobuf.Int64Value',
      },
      string: 'four',
      array: [5, 6],
      'null': null,
    }
  });
  response.send({
    data: {
      message: 'stub response',
      code: 42,
      long: {
        value: '420',
        '@type': 'type.googleapis.com/google.protobuf.Int64Value',
      },
    }
  });
});

exports.scalarTest = functions.https.onRequest((request, response) => {
  assert.deepEqual(request.body, { data: 17 });
  response.send({ data: 76 });
});

exports.tokenTest = functions.https.onRequest((request, response) => {
  assert.equal('Bearer token', request.get('Authorization'));
  assert.deepEqual(request.body, { data: {} });
  response.send({ data: {} });
});

exports.instanceIdTest = functions.https.onRequest((request, response) => {
  assert.equal(request.get('Firebase-Instance-ID-Token'), 'iid');
  assert.deepEqual(request.body, { data: {} });
  response.send({ data: {} });
});

exports.nullTest = functions.https.onRequest((request, response) => {
  assert.deepEqual(request.body, { data: null });
  response.send({ data: null });
});

exports.missingResultTest = functions.https.onRequest((request, response) => {
  assert.deepEqual(request.body, { data: null });
  response.send({});
});

exports.unhandledErrorTest = functions.https.onRequest((request, response) => {
  // Fail in a way that the client shouldn't see.
  throw 'nope';
});

exports.unknownErrorTest = functions.https.onRequest((request, response) => {
  // Send an http error with a body with an explicit code.
  response.status(400).send({
    error: {
      status: 'THIS_IS_NOT_VALID',
      message: 'this should be ignored',
    },
  });
});

exports.explicitErrorTest = functions.https.onRequest((request, response) => {
  // Send an http error with a body with an explicit code.
  // Note that eventually the SDK will have a helper to automatically return
  // the appropriate http status code for an error.
  response.status(400).send({
    error: {
      status: 'OUT_OF_RANGE',
      message: 'explicit nope',
      details: {
        start: 10,
        end: 20,
        long: {
          value: '30',
          '@type': 'type.googleapis.com/google.protobuf.Int64Value',
        },
      },
    },
  });
});

exports.httpErrorTest = functions.https.onRequest((request, response) => {
  // Send an http error with no body.
  response.status(400).send();
});