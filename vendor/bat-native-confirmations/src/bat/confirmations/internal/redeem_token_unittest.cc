/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "testing/gtest/include/gtest/gtest.h"

#include "base/files/file_path.h"
#include "base/strings/string_util.h"
#include "net/http/http_status_code.h"

#include "bat/confirmations/internal/confirmations_client_mock.h"
#include "bat/confirmations/internal/confirmations_impl_mock.h"
#include "bat/confirmations/internal/create_confirmation_request.h"
#include "bat/confirmations/internal/redeem_token_mock.h"
#include "bat/confirmations/internal/security_helper.h"
#include "bat/confirmations/internal/unblinded_tokens.h"

// npm run test -- brave_unit_tests --filter=Confirmations*

using ::testing::_;
using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::Return;

using std::placeholders::_1;

namespace confirmations {

class ConfirmationsRedeemTokenTest : public ::testing::Test {
 protected:
  ConfirmationsRedeemTokenTest()
      : confirmations_client_mock_(std::make_unique<
            NiceMock<MockConfirmationsClient>>()),
        confirmations_mock_(std::make_unique<
            NiceMock<ConfirmationsImplMock>>(confirmations_client_mock_.get())),
        unblinded_tokens_(std::make_unique<
            UnblindedTokens>(confirmations_mock_.get())),
        unblinded_payment_tokens_(std::make_unique<
            UnblindedTokens>(confirmations_mock_.get())),
        redeem_token_mock_(std::make_unique<
            NiceMock<RedeemTokenMock>>(confirmations_mock_.get(),
            confirmations_client_mock_.get(), unblinded_tokens_.get(),
            unblinded_payment_tokens_.get())) {
    // You can do set-up work for each test here
  }

  ~ConfirmationsRedeemTokenTest() override {
    // You can do clean-up work that doesn't throw exceptions here
  }

  // If the constructor and destructor are not enough for setting up and
  // cleaning up each test, you can use the following methods

  void SetUp() override {
    // Code here will be called immediately after the constructor (right before
    // each test)
    EXPECT_CALL(*confirmations_client_mock_, LoadState(_, _))
        .WillRepeatedly(
            Invoke([this](
                const std::string& name,
                LoadCallback callback) {
              base::FilePath path = GetTestDataPath();
              path = path.AppendASCII(name);

              std::string value;
              if (!Load(path, &value)) {
                callback(FAILED, value);
                return;
              }

              callback(SUCCESS, value);
            }));

    ON_CALL(*confirmations_client_mock_, SaveState(_, _, _))
        .WillByDefault(
            Invoke([](
                const std::string& name,
                const std::string& value,
                ResultCallback callback) {
              callback(SUCCESS);
            }));

    const auto callback = std::bind(
        &ConfirmationsRedeemTokenTest::OnInitialize, this, _1);
    confirmations_mock_->Initialize(callback);
  }

  void OnInitialize(
      const bool success) {
    EXPECT_TRUE(success);
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right before the
    // destructor)
  }

  // Objects declared here can be used by all tests in the test case

  base::FilePath GetTestDataPath() {
    return base::FilePath("brave/vendor/bat-native-confirmations/test/data");
  }

  bool Load(
      const base::FilePath path,
      std::string* value) {
    if (!value) {
      return false;
    }

    std::ifstream ifs{path.value().c_str()};
    if (ifs.fail()) {
      *value = "";
      return false;
    }

    std::stringstream stream;
    stream << ifs.rdbuf();
    *value = stream.str();
    return true;
  }

  ConfirmationInfo CreateConfirmationInfo() {
    ConfirmationInfo confirmation;

    confirmation.id = "f00bbf3b-e5e4-43b7-adec-5d9110d1765d";

    confirmation.creative_instance_id = "a6122ca5-3d21-4d32-9e88-d6eb457f07e7";

    confirmation.type = ConfirmationType::kViewed;

    const TokenInfo token = unblinded_tokens_->GetToken();
    unblinded_tokens_->RemoveToken(token);
    confirmation.token_info = token;

    const std::string payment_token_base64 = R"(zoxucKcRIwsUP+PU3tM/GE0fbG0ERRyuPPMwFhMOYp9Ci5fN8eB4hDZHj0bAeo6g29dU4qYVCWMVT+M/3jGAODWwu+uFzBN37QsCC3ZES2fVwpq6Pf8Lh+vGaHSNyM8G)";
    confirmation.payment_token = Token::decode_base64(payment_token_base64);

    const std::string blinded_payment_token_base64 = R"(QnDCPeQi38eBSxbhTHa7uHsju30EE0hwC5uojatn6DA=)";
    confirmation.blinded_payment_token =
        BlindedToken::decode_base64(blinded_payment_token_base64);

    const CreateConfirmationRequest request;
    const std::string payload =
        request.CreateConfirmationRequestDTO(confirmation);
    confirmation.credential = request.CreateCredential(token, payload);
    confirmation.timestamp_in_seconds = 1587127747;

    return confirmation;
  }

  std::unique_ptr<MockConfirmationsClient> confirmations_client_mock_;
  std::unique_ptr<ConfirmationsImplMock> confirmations_mock_;

  std::unique_ptr<UnblindedTokens> unblinded_tokens_;
  std::unique_ptr<UnblindedTokens> unblinded_payment_tokens_;

  std::unique_ptr<RedeemTokenMock> redeem_token_mock_;
};

TEST_F(ConfirmationsRedeemTokenTest,
    SuccessfullyRedeemToken) {
  // Arrange
  ON_CALL(*confirmations_client_mock_, LoadURL(_, _, _, _, _, _))
      .WillByDefault(Invoke([](
          const std::string& url,
          const std::vector<std::string>& headers,
          const std::string& content,
          const std::string& content_type,
          const ledger::UrlMethod method,
          URLRequestCallback callback) {
        std::string response = "";
        int response_status_code = -1;

        const std::string request_signed_tokens_endpoint = R"(/v1/confirmation/token/88cfda43-c801-4464-b249-5abacbd182a0)";
        const std::string get_signed_tokens_endpoint = R"(/v1/confirmation/token/88cfda43-c801-4464-b249-5abacbd182a0?nonce=9eed0b96-599f-44e8-95e6-9fe89e84f640)";
        const std::string create_confirmation_endpoint = R"(/v1/confirmation/f00bbf3b-e5e4-43b7-adec-5d9110d1765d/eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlblwiOlwiUW5EQ1BlUWkzOGVCU3hiaFRIYTd1SHNqdTMwRUUwaHdDNXVvamF0bjZEQT1cIixcImNyZWF0aXZlSW5zdGFuY2VJZFwiOlwiYTYxMjJjYTUtM2QyMS00ZDMyLTllODgtZDZlYjQ1N2YwN2U3XCIsXCJwYXlsb2FkXCI6e30sXCJ0eXBlXCI6XCJ2aWV3XCJ9Iiwic2lnbmF0dXJlIjoiODZ0YTZYSkhwNlB0NENCNXN2aUlVellvZlNkT0hGRFVXNTU2K0piMHVOK3pMSmZRU3J5RzNuUXJlMi9ldjd2K09wTmN4RURYbjFaRm1oYk1zTkFURlE9PSIsInQiOiIvSkk2bEpTTHJQRzFycWdxcDRoODk1N2lKQWlNd0YxRVRReUlSczNjSDBSVVN1d2lITXkxbDNWSFRsenk4Sm5DZzVVYzlPWnlpaW05TlZhZzF0ZHVEZz09In0=)";
        const std::string fetch_payment_token_endpoint = R"(/v1/confirmation/f00bbf3b-e5e4-43b7-adec-5d9110d1765d/paymentToken)";

        if (base::EndsWith(url, request_signed_tokens_endpoint,
            base::CompareCase::INSENSITIVE_ASCII)) {
          response = R"({"nonce":"9eed0b96-599f-44e8-95e6-9fe89e84f640"})";
          response_status_code = net::HTTP_CREATED;
        } else if (base::EndsWith(url, get_signed_tokens_endpoint,
            base::CompareCase::INSENSITIVE_ASCII)) {
          response = R"({"batchProof":"d3pfqJWh03Mse2I2i26+5UALFztGGNlaT/sQK43XtAn5HeTxDZ7sQfqBmbmcXHFO2NiaiO8+SagVmtnxeexSCg==","signedTokens":["RlUN4TRorM/DYSxsoro3OpsnbJrWmU0KkH4AxqKi2yI=","yjKg69xGToG5xhxmTnaWOfcCzjXs6J6uut7kKG2Qdxk=","2OlcOSB5QPene190Ey7iKjk1q4ojVhtnsyzSq1CpQCs=","dHMncmLh+B7jPlBV3YC0gasWCA4j4UinADs4JO6hz2o=","1EirkPIIEdvPsB2JGJzs+4DaDsfsGfKVulvI3o9p5Q8=","7EuoBnroVTDfFR9nB1ELxXXHEQxYLdAeHCDsLghj3yg=","gmpQ5k8eFsd2V+Yy23cFNrzVMGvzKabtg1HFzDnN6kA=","vNNLfNL1m7W/KwqEeIEO39Ncyr4KNWJAbNFMxUi1T2g=","xubFyyYUGAd71YI+klv3kW5Gf30Zt0d4z7gQvg7qWV4=","aloI3ySRYKMwwYggfwH0B4BxYiwgPWuP3sGVFZTuoHY=","bp7ozMlzCL0Apb3eaGZVeeCaQCthFXlG1bfTCzXU4BU=","gDuvy1Nv3M16y0YFW2iyD83RktD1/Wa0E0UEiPFNEns=","iOSbdZV1CJ7uSyVDVZmTd9dOr02cJIX8UBn7/tzd9zM=","Ehv+3++Hj/zEAxiole9TRLMd9gtTPUlmLH9A3zxtDRM=","2ip0ei5mlK+RqjxtS/TswXRC4i7Ap/8sSDeYwwVBnUI=","6iuWMaL+EmghUtB/ACbmIjCchtKT2PrU1+lVr21am2s=","2FMe4sgYOBsBrRA2aMZ1MlDfORWN017GWz8/xsxrLU8=","qvyiEcVg0dAzxLGWr/8K/z3qZv3vOZvefwXkl1UChQ4=","YAJ5B8kOtCHxxT4v3lyy3yfqODrrnTtuBuMWuOXeqgg=","qJSyNoCVtDAZxKW2tP2RamDm8QIp1bgSgqT/GK89RQA=","frFOsgCTwZhdZNzocvkhuRyL2CmpaYS21TNsAIDIeCU=","5EdS2wsMogTQ4LqgJKE1Xy/sMTZDqz5fdg+I7o/M8kE=","hl+ciyLm5oH0LCBwpHM3uQyrkBU9AXfz0MJz5nIHfG8=","nko7oxhHfkzIq0s6vty/FD5Sp6clbFaCs4C26l5gPAA=","vHKEiTQSw9PUdVr/z/ChBkm5Sdhb4KZZtOEwNo11+jY=","Mk2IJnb/w+7VFacr06+ray+1XnhtLsYpY4otAsnCy0k=","uiWHBu/N9QzQnx2NQrmRDdFPLrIOgrwYXhIOPoZFAwI=","Im9hqnvTsxy8uQenuJfsybCnZm++rGQbkOeALTdy+yM=","1NFCgyw7XL/TIiJLUFE4tF8CQip5w3unt9JJCLdw+Ao=","dH5Hm2ZAZV3/sp2F8B6mfv1ZaT9xoV3mnF/MSsxbdFQ=","GAw3Jb0U17HYBg5vqhDlb6vQgo7MJOmbAoF3MOjb/jM=","3BTd0AYax4MFoZR/ZxcFXTLmsdBWBU3yl77ggabOVQA=","PmuqvNpgHbAtDSyMJ+OVHLMlSMEVJ8zmsnX33nu7wRI=","ZmUA4GrTcbjVV18q0oGtLoJ4Snl7fAsdPrto66esrjA=","rE7PMwYE6qoUzrklLgO/ctFcXfyiFuqNmey2OwDMUTc=","gEb6pACH2xzpzis2DhnNcTa1J0snuOxVr1rLO8qHZ28=","vBqEvGGpCQa+MzSb8FmxeU3BaypcFC2ZwSpT8WUCIzE=","iFL1w8M1WVnJ1pRF1BxogPyBx+0tGESNW5rThNV9238=","9qc4bvToGz0ZijobbKL6TbbKdEOaSa2vm4uhY0QOoz8=","zowOqjZIx5VtM8eCblax+IIQWio0dYSVAuWTEhXPzkk=","iCnv0MI4XcWVC9MOAWqBU95rs4Q9SJuvvQQVzwdWmhw=","YLubzD4yac+hxuJNehnYt3wIRtboWEI7DC/TkmsVaDM=","7pjpDr86qoiZXwpy0rxaZFjY1YoFIDI3Nm2xJpVRSxM=","oFniod6gj4CDZXSoO6LPm9AefjrTeNGdVSwjNr1m0h8=","HH8jHatXuWO5WNGZrhFAiyC7EnATFokzIoplsZ6stjs=","DLD6Lkm5dHnYJZAlcNtWvbcJ1oH/P3a8NAxjjBNU6GA=","qBj1zj7/vGffaf80szzNvAs/N97GJxgXHJlQh9XZyFI=","tv27GayeW6einr4FW0n56e08MeGhkRLa+SY6Ij9ND3E=","dqeYnNnzPyHIikq2y5cSYeMit5XuQvHIJDN82/+DiXw=","5BmtBFW3mE7bv/ptD9B+PMxFyte5+9rTtt9feCRON2g="],"publicKey":"crDVI1R6xHQZ4D9cQu4muVM5MaaM1QcOT4It8Y/CYlw="})";
          response_status_code = net::HTTP_OK;
        } else if (base::EndsWith(url, create_confirmation_endpoint,
            base::CompareCase::INSENSITIVE_ASCII)) {
          response = R"({"id":"f00bbf3b-e5e4-43b7-adec-5d9110d1765d","payload":{},"createdAt":"2020-04-17T12:49:07.243Z","type":"view","modifiedAt":"2020-04-17T12:49:07.243Z","creativeInstanceId":"a6122ca5-3d21-4d32-9e88-d6eb457f07e7"})";
          response_status_code = net::HTTP_CREATED;
        } else if (base::EndsWith(url, fetch_payment_token_endpoint,
            base::CompareCase::INSENSITIVE_ASCII)) {
          response = R"({"id":"f00bbf3b-e5e4-43b7-adec-5d9110d1765d","createdAt":"2020-04-17T12:49:07.243Z","type":"view","modifiedAt":"2020-04-17T12:49:07.261Z","creativeInstanceId":"a6122ca5-3d21-4d32-9e88-d6eb457f07e7","paymentToken":{"publicKey":"bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU=","batchProof":"yR583JEiAx4DGNTon6YDPy2Ac71yOh9HCznzk+2pFQG7tdlKoIl3UNXpNir1q9aj7BYAD5cJMAFYWPqNeXzQCw==","signedTokens":["0sFb6JSxdaJ+fZvgoJoTkC7WKTdI3KFXMK0aHFizs34="]}})";
          response_status_code = net::HTTP_OK;
        }

        callback(response_status_code, response, {});
      }));

  TokenInfo token;
  const std::string unblinded_token_base64 = R"(/JI6lJSLrPG1rqgqp4h8957iJAiMwF1ETQyIRs3cH0RUSuwiHMy1l3VHTlzy8JnCg5Uc9OZyiim9NVag1tduDi4CbEG9DjWD6xGhvmO3pLLd7FdqpTEvXBs45er5S3kL)";
  token.unblinded_token = UnblindedToken::decode_base64(unblinded_token_base64);
  token.public_key = "crDVI1R6xHQZ4D9cQu4muVM5MaaM1QcOT4It8Y/CYlw=";
  unblinded_tokens_->SetTokens({token});

  // Act
  const Result expected_result = SUCCESS;
  const bool expected_should_retry = false;

  EXPECT_CALL(*redeem_token_mock_, OnRedeem(expected_result, _,
       expected_should_retry)).Times(1);

  const ConfirmationInfo confirmation = CreateConfirmationInfo();
  redeem_token_mock_->Redeem(confirmation);

  // Assert
}

}  // namespace confirmations
